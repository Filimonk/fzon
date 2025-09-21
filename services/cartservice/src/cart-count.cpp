#include <cart-count.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/formats/json.hpp>
#include <userver/clients/http/component.hpp>

namespace cartservice {

CartCount::CartCount(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string CartCount::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    LOG_INFO() << "Hi1";
    // Проверяем заголовок Authorization
    const auto auth_header = request.GetHeader("Authorization");
    if (auth_header.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return "";
    }
    LOG_INFO() << "Hi2";

    try {
        // Проверяем JWT токен через authservice
        auto response = http_client_.CreateRequest()
            .get()
            .url("http://authservice:8080/verify/")
            .headers({{"Authorization", auth_header}})
            .timeout(std::chrono::seconds(2))
            .perform();
        LOG_INFO() << "Hi3";

        if (response->status_code() != 200) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return "";
        }
        LOG_INFO() << "Hi4";

        // Парсим ответ от authservice и извлекаем user_id
        const auto json_body = userver::formats::json::FromString(response->body());
        const auto user_id = json_body["user_id"].As<int>();
        LOG_INFO() << "Hi5";

        // Суммируем количество товаров в корзине
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "SELECT SUM(quantity) as total_count FROM cart WHERE user_id = $1",
            user_id
        );
        LOG_INFO() << "Hi6";

        int cart_count = 0;
        if (!result[0]["total_count"].IsNull()) {
            LOG_INFO() << "Hi7";
            cart_count = result[0]["total_count"].As<int>();
        }
        LOG_INFO() << "Hi8";

        // Формируем ответ
        userver::formats::json::ValueBuilder response_json;
        response_json["cartCount"] = cart_count;
        LOG_INFO() << "Hi9";

        request.GetHttpResponse().SetContentType("application/json");
        return userver::formats::json::ToString(response_json.ExtractValue());

    } catch (const std::exception& ex) {
        LOG_INFO() << "Hi10";
        LOG_ERROR() << "Error while getting cart count: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "";
    }
}

}  // namespace cartservice

