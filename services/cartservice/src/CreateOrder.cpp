#include <CreateOrder.hpp>

#include <utility>
#include <unordered_map>

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

        // Получаем корзину пользователя
        auto cart_result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "SELECT article, quantity FROM cart WHERE user_id = $1",
            user_id
        );

        if (cart_result.Size() == 0) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error": "Cart is empty"})";
        }

        // Собираем артикулы для запроса в catalogservice
        userver::formats::json::ValueBuilder catalog_request;
        userver::formats::json::ValueBuilder articles_json;

        for (const auto& row : cart_result) {
            articles_json.PushBack(row["article"].As<std::string>());
        }
        catalog_request["articles"] = articles_json;

        // Запрашиваем цены у catalogservice
        auto catalog_response = http_client_.CreateRequest()
            .post()
            .url("http://catalogservice:8080/fetch-prices-bulk")
            // .headers({{"Authorization", auth_header}})
            .data(userver::formats::json::ToString(catalog_request.ExtractValue()))
            .timeout(std::chrono::seconds(3))
            .perform();

        if (catalog_response->status_code() != 200) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kFailedDependency);
            return R"({"error": "Failed to fetch prices from catalogservice"})";
        }

        const auto catalog_json = userver::formats::json::FromString(catalog_response->body());
        std::unordered_map<std::string, double> price_map;
        for (const auto& price_entry : catalog_json["prices"]) {
            price_map[price_entry["article"].As<std::string>()] =
                price_entry["price"].As<double>();
        }

        // Формируем JSON корзины с ценами
        userver::formats::json::ValueBuilder cart_items;
        std::vector<std::pair<std::string, int>> articles_to_remove;

        for (const auto& row : cart_result) {
            userver::formats::json::ValueBuilder item;
            const auto article = row["article"].As<std::string>();
            const auto quantity = row["quantity"].As<int>();

            item["article"] = article;
            item["quantity"] = quantity;

            if (price_map.find(article) != price_map.end()) {
                item["price"] = price_map[article];
            } else {
                // Если для артикула не пришла цена
                request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
                return R"({"error": "Some articles missing prices"})";
            }

            cart_items.PushBack(std::move(item));
            articles_to_remove.push_back({article, quantity});
        }

        // Финальный JSON для orderservice
        userver::formats::json::ValueBuilder order_request;
        order_request["cart_items"] = cart_items;

        auto order_response = http_client_.CreateRequest()
            .post()
            .url("http://orderservice:8080/create-order")
            .headers({
                {"Authorization", auth_header}
            })
            .data(userver::formats::json::ToString(order_request.ExtractValue()))
            .timeout(std::chrono::seconds(5))
            .perform();

        if (order_response->status_code() != 204) {
            request.SetResponseStatus(
                static_cast<userver::server::http::HttpStatus>(order_response->status_code()));
            return order_response->body();
        }

        // Чистим корзину
        for (const auto& [article, quantity] : articles_to_remove) {
            auto result = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "SELECT quantity FROM cart WHERE user_id = $1 AND article = $2",
                user_id, article
            );

            if (result.Size() > 0) {
                const auto current_quantity = result[0]["quantity"].As<int>();
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
        return R"({"error": "Internal server error"})";
    }
}

}  // namespace cartservice

