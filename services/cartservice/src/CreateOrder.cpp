#include <CreateOrder.hpp>

#include <utility> 

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/clients/http/component.hpp>

namespace cartservice {

CreateOrder::CreateOrder(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string CreateOrder::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    // Проверяем авторизацию
    const auto auth_header = request.GetHeader("Authorization");
    if (auth_header.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return "";
    }

    // Получаем токен идемпотентности
    const auto idempotency_token = request.GetHeader("Idempotency-Token");
    if (idempotency_token.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return "{\"error\": \"Idempotency-Token header is required\"}";
    }

    try {
        // Проверяем JWT токен через authservice
        auto auth_response = http_client_.CreateRequest()
            .get()
            .url("http://authservice:8080/verify/")
            .headers({{"Authorization", auth_header}})
            .timeout(std::chrono::seconds(2))
            .perform();

        if (auth_response->status_code() != 200) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return "";
        }

        // Парсим ответ от authservice и извлекаем user_id
        const auto json_body = userver::formats::json::FromString(auth_response->body());
        const auto user_id = json_body["user_id"].As<int>();

        // Получаем всю корзину пользователя
        auto cart_result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "SELECT article, quantity FROM cart WHERE user_id = $1",
            user_id
        );

        if (cart_result.Size() == 0) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "{\"error\": \"Cart is empty\"}";
        }

        // Формируем JSON с корзиной для передачи в orderservice
        userver::formats::json::ValueBuilder cart_items;
        std::vector<std::pair <std::string, int> > articles_to_remove;

        for (const auto& row : cart_result) {
            userver::formats::json::ValueBuilder item;
            const auto article = row["article"].As<std::string>();
            const auto quantity = row["quantity"].As<int>();

            item["article"] = article;
            item["quantity"] = quantity;
            cart_items.PushBack(std::move(item));

            articles_to_remove.push_back({article, quantity});
        }

        // Формируем запрос к orderservice
        userver::formats::json::ValueBuilder order_request;
        order_request["cart_items"] = cart_items;
        order_request["idempotency_token"] = idempotency_token;

        // Отправляем запрос в orderservice
        auto order_response = http_client_.CreateRequest()
            .post()
            .url("http://orderservice:8080/create-order")
            .headers({
                {"Authorization", auth_header},
                {"Idempotency-Token", idempotency_token}
            })
            .data(userver::formats::json::ToString(order_request.ExtractValue()))
            .timeout(std::chrono::seconds(5))
            .perform();

        if (order_response->status_code() != 204) {
            request.SetResponseStatus(
                static_cast<userver::server::http::HttpStatus>(order_response->status_code()));
            return order_response->body();
        }

        // Если заказ успешно создан, очищаем корзину (только переданные товары)
        for (const auto& [article, quantity] : articles_to_remove) {
            auto result = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "SELECT quantity FROM cart WHERE user_id = $1 AND article = $2",
                user_id, article
            );

            if (result.Size() > 0) {
                const auto current_quantity = result[0]["quantity"].As<int>();
                // Уменьшаем количество или удаляем строку
                if (current_quantity - quantity <= 0) {
                    pg_cluster_->Execute(
                        userver::storages::postgres::ClusterHostType::kMaster,
                        "DELETE FROM cart WHERE user_id = $1 AND article = $2",
                        user_id, article
                    );
                } else {
                    pg_cluster_->Execute(
                        userver::storages::postgres::ClusterHostType::kMaster,
                        "UPDATE cart SET quantity = quantity - $3 WHERE user_id = $1 AND article = $2",
                        user_id, article, quantity
                    );
                }
            }
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while creating order: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "{\"error\": \"Internal server error\"}";
    }
}

}  // namespace cartservice

