#include <CreateOrder.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/storages/postgres/component.hpp>

namespace orderservice {

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

        // Получаем карзину пользователя
        const auto body_json = userver::formats::json::FromString(request.RequestBody());
        const auto cart_items = body_json["cart_items"];

        if (!cart_items.IsArray() || cart_items.GetSize() == 0) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error": "Cart items are required"})";
        }

        // считаем сумму заказа
        double total_amount = 0.0;
        for (const auto& item : cart_items) {
            total_amount += item["price"].As<double>() * item["quantity"].As<int>();
        }

        // сохраняем в outbox
        pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO outbox (user_id, payload) VALUES ($1, $2::jsonb)",
            user_id,
            body_json
        );

        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while creating order: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return R"({"error": "Internal server error"})";
    }
}

}  // namespace orderservice
