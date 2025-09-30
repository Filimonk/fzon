#include "ChangeCartProductCountByUserId.hpp"

#include <userver/storages/postgres/component.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/formats/json.hpp>

namespace cartservice {

ChangeCartProductCountByUserId::ChangeCartProductCountByUserId(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string ChangeCartProductCountByUserId::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {

    try {
        // Парсим тело запроса
        const auto request_json = userver::formats::json::FromString(request.RequestBody());

        if (!request_json.HasMember("userId") || !request_json.HasMember("article") || !request_json.HasMember("productQuantity")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "";
        }

        const auto user_id = request_json["userId"].As<int>();
        const auto article = request_json["article"].As<std::string>();
        const auto product_quantity = request_json["productQuantity"].As<int>();

        // Проверяем существование записи в корзине
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "SELECT quantity FROM cart WHERE user_id = $1 AND article = $2",
            user_id, article
        );

        if (result.Size() > 0) {
            // Запись существует - прибавляем количество
            const auto current_quantity = result[0]["quantity"].As<int>();
            const auto new_quantity = current_quantity + product_quantity;

            if (new_quantity <= 0) {
                // Удаляем запись, если новое количество <= 0
                pg_cluster_->Execute(
                    userver::storages::postgres::ClusterHostType::kMaster,
                    "DELETE FROM cart WHERE user_id = $1 AND article = $2",
                    user_id, article
                );
            } else {
                // Обновляем количество на сумму
                pg_cluster_->Execute(
                    userver::storages::postgres::ClusterHostType::kMaster,
                    "UPDATE cart SET quantity = $1 WHERE user_id = $2 AND article = $3",
                    new_quantity, user_id, article
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
        
        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while changing cart product count by user id: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "";
    }
}

}  // namespace cartservice

