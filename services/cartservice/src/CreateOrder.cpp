#include <CreateOrder.hpp>

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
        const auto articles = request_json["articles"].As<std::vector<std::string>>();

        std::string query = "SELECT article, quantity FROM cart WHERE user_id = $1";
        if (!articles.empty()) {
            query += " AND article = ANY($2)";
        }

        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            query,
            user_id,
            articles
        );

        // Формируем ответ
        userver::formats::json::ValueBuilder cart_items_builder(
            userver::formats::common::Type::kArray
        );

        for (const auto& row : result) {
            userver::formats::json::ValueBuilder item_builder;
            item_builder["article"] = row["article"].As<std::string>();
            item_builder["quantity"] = row["quantity"].As<int>();
            cart_items_builder.PushBack(item_builder.ExtractValue());
        }
        
        userver::formats::json::ValueBuilder response_builder;
        response_builder["cartItems"] = cart_items_builder;

        request.GetHttpResponse().SetContentType("application/json");
        return userver::formats::json::ToString(response_builder.ExtractValue());

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while fetching cart items bulk: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "";
    }
}

}  // namespace cartservice

