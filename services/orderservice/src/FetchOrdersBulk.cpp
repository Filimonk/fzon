#include <FetchOrdersBulk.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/storages/postgres/component.hpp>

namespace orderservice {

FetchOrdersBulk::FetchOrdersBulk(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string FetchOrdersBulk::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&
) const {
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

        const auto json_body = userver::formats::json::FromString(auth_response->body());
        const auto user_id = json_body["user_id"].As<int>();

        // Получаем заказы пользователя
        auto res_orders = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT id, total_amount::float8, status, created_at "
            "FROM orderserviceschema.orders WHERE user_id=$1 ORDER BY created_at DESC",
            user_id
        );

        userver::formats::json::ValueBuilder response_json(userver::formats::json::Type::kObject);
        auto orders_array = response_json["orders"];

        for (const auto& row : res_orders) {
            userver::formats::json::ValueBuilder order_json(userver::formats::json::Type::kObject);

            int order_id = row["id"].As<int>();
            order_json["order_id"] = order_id;
            order_json["total_amount"] = row["total_amount"].As<double>();
            order_json["status"] = row["status"].As<std::string>();
            auto created_at = row["created_at"].As<userver::storages::postgres::TimePointTz>();
            order_json["created_at"] = userver::utils::datetime::Timestring(created_at.GetUnderlying());

            // Получаем items для заказа
            auto res_items = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kSlave,
                "SELECT article, quantity, price::float8 "
                "FROM orderserviceschema.order_items WHERE order_id=$1",
                order_id
            );

            auto items_array = order_json["items"];
            for (const auto& item_row : res_items) {
                userver::formats::json::ValueBuilder item_json(userver::formats::json::Type::kObject);
                item_json["article"] = item_row["article"].As<std::string>();
                item_json["quantity"] = item_row["quantity"].As<int>();
                item_json["price"] = item_row["price"].As<double>();
                items_array.PushBack(item_json.ExtractValue());
            }

            orders_array.PushBack(order_json.ExtractValue());
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
        return userver::formats::json::ToString(response_json.ExtractValue());

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while fetching orders: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return R"({"error": "Internal server error"})";
    }
}

} // namespace orderservice

