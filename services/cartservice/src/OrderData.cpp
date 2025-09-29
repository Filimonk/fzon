#include <OrderData.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace cartservice {

OrderData::OrderData(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string OrderData::
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

        // Получаем все товары в корзине пользователя
        auto cart_result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "SELECT article, quantity FROM cart WHERE user_id = $1",
            user_id
        );

        // Если корзина пуста, возвращаем нулевые значения
        if (cart_result.Size() == 0) {
            userver::formats::json::ValueBuilder response_json;
            response_json["cartCount"] = 0;
            response_json["sum"] = 0.0;

            request.GetHttpResponse().SetContentType("application/json");
            return userver::formats::json::ToString(response_json.ExtractValue());
        }

        // Формируем список артикулов для запроса к catalogservice
        std::vector<std::string> articles;
        std::unordered_map<std::string, int> article_quantities;

        for (const auto& row : cart_result) {
            const auto article = row["article"].As<std::string>();
            const auto quantity = row["quantity"].As<int>();
            articles.push_back(article);
            article_quantities[article] = quantity;
        }

        // Запрашиваем цены товаров из catalogservice
        userver::formats::json::ValueBuilder request_builder;
        request_builder["articles"] = articles;

        auto price_response = http_client_.CreateRequest()
            .post()
            .url("http://catalogservice:8080/fetch-prices-bulk")
            .data(userver::formats::json::ToString(request_builder.ExtractValue()))
            .timeout(std::chrono::seconds(2))
            .perform();

        if (price_response->status_code() != 200) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kFailedDependency);
            return R"({"error": "Failed to fetch prices from catalogservice"})";
        }

        const auto price_json = userver::formats::json::FromString(price_response->body());
        std::unordered_map<std::string, double> prices;
        for (const auto& price_entry : price_json["prices"]) {
            prices[price_entry["article"].As<std::string>()] =
                price_entry["price"].As<double>();
        }

        // Вычисляем общее количество и сумму
        int cart_count = 0;
        double total_sum = 0.0;

        for (const auto& article : articles) {
            const auto quantity = article_quantities[article];
            cart_count += quantity;

            // Ищем цену товара в ответе
            if (prices.find(article) != prices.end()) {
                const auto price = prices[article];
                total_sum += price * quantity;
            } else {
                LOG_ERROR() << "Price not found for article: " << article;
                request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
                return "{\"error\": \"Missing product price\"}";
            }
        }

        // Формируем ответ
        userver::formats::json::ValueBuilder response_json;
        response_json["cartCount"] = cart_count;
        response_json["sum"] = total_sum;

        request.GetHttpResponse().SetContentType("application/json");
        return userver::formats::json::ToString(response_json.ExtractValue());

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while getting order data: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "";
    }
}

}  // namespace cartservice

