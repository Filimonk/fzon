#include <ChangeCartProductCount.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/formats/json.hpp>
#include <userver/clients/http/component.hpp>

namespace cartservice {

ChangeCartProductCount::ChangeCartProductCount(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string ChangeCartProductCount::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    // Проверяем заголовок Authorization
    const auto auth_header = request.GetHeader("Authorization");
    if (auth_header.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return "";
    }

    try {
        // Проверяем JWT токен через authservice
        auto response = http_client_.CreateRequest()
            .get()
            .url("http://authservice:8080/verify/")
            .headers({{"Authorization", auth_header}})
            .timeout(std::chrono::seconds(2))
            .perform();

        if (response->status_code() != 200) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return "";
        }

        // Парсим ответ от authservice и извлекаем user_id
        const auto json_body = userver::formats::json::FromString(response->body());
        const auto user_id = json_body["user_id"].As<int>();

        // Парсим тело запроса
        const auto request_json = userver::formats::json::FromString(request.RequestBody());

        if (!request_json.HasMember("article") || !request_json.HasMember("productQuantity")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "";
        }

        const auto article = request_json["article"].As<std::string>();
        const auto product_quantity = request_json["productQuantity"].As<int>();

        // Проверяем существование записи в корзине
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "SELECT quantity FROM cart WHERE user_id = $1 AND article = $2",
            user_id, article
        );

        if (result.Size() > 0) {
            // Запись существует
            if (product_quantity <= 0) {
                // Удаляем запись, если количество <= 0
                pg_cluster_->Execute(
                    userver::storages::postgres::ClusterHostType::kMaster,
                    "DELETE FROM cart WHERE user_id = $1 AND article = $2",
                    user_id, article
                );
            } else {
                // Обновляем количество
                pg_cluster_->Execute(
                    userver::storages::postgres::ClusterHostType::kMaster,
                    "UPDATE cart SET quantity = $1 WHERE user_id = $2 AND article = $3",
                    product_quantity, user_id, article
                );
            }
        } else if (product_quantity > 0) {
            // Записи нет, но productQuantity положительная - создаем новую запись
            pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "INSERT INTO cart (user_id, article, quantity) VALUES ($1, $2, $3)",
                user_id, article, product_quantity
            );
        }

        // Возвращаем успешный ответ без тела
        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while changing cart product count: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "";
    }
}

}  // namespace cartservice

