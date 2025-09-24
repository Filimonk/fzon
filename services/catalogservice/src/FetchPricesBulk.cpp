#include <FetchPricesBulk.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace catalogservice {

FetchPricesBulk::FetchPricesBulk(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string FetchPricesBulk::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    try {
        // Парсим тело запроса
        const auto request_json = userver::formats::json::FromString(request.RequestBody());
        const auto articles = request_json["articles"].As<std::vector<std::string>>();

        if (articles.empty()) {
            userver::formats::json::ValueBuilder response;
            response["prices"] = userver::formats::json::ValueBuilder();
            
            request.GetHttpResponse().SetContentType("application/json");
            return userver::formats::json::ToString(response.ExtractValue());
        }

        // Запрашиваем цены товаров
        std::string query = "SELECT article, price::float8 FROM products WHERE article = ANY($1)";
        
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            query,
            articles
        );

        // Формируем ответ
        userver::formats::json::ValueBuilder prices_builder;
        
        for (const auto& row : result) {
            const auto article = row["article"].As<std::string>();
            const auto price = row["price"].As<double>();
            prices_builder[article] = price;
        }

        userver::formats::json::ValueBuilder response_builder;
        response_builder["prices"] = prices_builder;
        
        request.GetHttpResponse().SetContentType("application/json");
        return userver::formats::json::ToString(response_builder.ExtractValue());

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while fetching prices: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "{\"error\": \"Internal server error\"}";
    }
}

}  // namespace catalogservice

