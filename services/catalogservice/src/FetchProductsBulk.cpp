#include <FetchProductsBulk.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace catalogservice {

FetchProductsBulk::FetchProductsBulk(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string FetchProductsBulk::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    try {
        // Получаем все товары из базы данных
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "SELECT article, name, price::float8, seller_name, rating::float8 FROM products"
        );

        // Собираем список артикулов для запроса к сервису корзины
        std::vector<std::string> articles;
        for (const auto& row : result) {
            articles.push_back(row["article"].As<std::string>());
        }

        // Проверяем авторизацию пользователя
        std::unordered_map<std::string, int> cart_quantities;
        const auto auth_header = request.GetHeader("Authorization");

        if (!auth_header.empty()) {
            try {
                // Запрашиваем данные о корзине из сервиса корзины
                userver::formats::json::ValueBuilder request_builder;
                request_builder["articles"] = articles;

                auto cart_response = http_client_.CreateRequest()
                    .post()
                    .url("http://cartservice:8080/cart-items-bulk")
                    .headers({{"Authorization", auth_header}})
                    .data(userver::formats::json::ToString(request_builder.ExtractValue()))
                    .timeout(std::chrono::seconds(2))
                    .perform();

                if (cart_response->status_code() == 200) {
                    // Парсим ответ от сервиса корзины
                    const auto cart_json = userver::formats::json::FromString(cart_response->body());
                    const auto cart_items = cart_json["cartItems"];

                    for (const auto& item : cart_items) {
                        const auto article = item["article"].As<std::string>();
                        const auto quantity = item["quantity"].As<int>();
                        cart_quantities[article] = quantity;
                    }
                }
            } catch (const std::exception& ex) {
                LOG_ERROR() << "Error while requesting cart service: " << ex.what();
                // Продолжаем выполнение с пустой корзиной
            }
        }

        // Формируем ответ
        userver::formats::json::ValueBuilder products_builder;

        for (const auto& row : result) {
            const auto article = row["article"].As<std::string>();
            const auto quantity_it = cart_quantities.find(article);
            const auto quantity = (quantity_it != cart_quantities.end()) ? quantity_it->second : 0;

            userver::formats::json::ValueBuilder product_builder;
            product_builder["price"] = (row["price"].As<double>());
            product_builder["sellerName"] = row["seller_name"].As<std::string>();
            product_builder["name"] = row["name"].As<std::string>();
            product_builder["rating"] = (row["rating"].As<double>());
            product_builder["productQuantity"] = quantity;

            products_builder[article] = product_builder;
        }

        userver::formats::json::ValueBuilder response_builder;
        response_builder["products"] = products_builder;

        request.GetHttpResponse().SetContentType("application/json");
        return userver::formats::json::ToString(response_builder.ExtractValue());

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while fetching products: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "{\"error\": \"Internal server error\"}";
    }
}

}  // namespace catalogservice

