#include <FetchProductsBulk.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/http/http_status.hpp>

namespace catalogservice {

FetchProductsBulk::FetchProductsBulk(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string FetchProductsBulk::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    try {
        return 
"{\n"
"    \"products\": {\n"
"        \"1\": {\n"
"            \"price\": 59595959.123123,\n"
"            \"sellerName\": \"Fzon\",\n"
"            \"name\": \"Tecno Смартфон SPARK 30C 6/128 ГБ, черный\",\n"
"            \"rating\": 4,\n"
"            \"productQuantity\": 0\n"
"        },\n"
"        \"2\": {\n"
"            \"price\": 12999,\n"
"            \"sellerName\": \"Fzon\",\n"
"            \"name\": \"Xiaomi Redmi Note 12 6/128 ГБ, синий\",\n"
"            \"rating\": 4.7,\n"
"            \"productQuantity\": 2\n"
"        },\n"
"        \"3\": {\n"
"            \"price\": 8999.1,\n"
"            \"sellerName\": \"TechStore\",\n"
"            \"name\": \"Realme C55 6/128 ГБ, солнечный желтый\",\n"
"            \"rating\": 4.8,\n"
"            \"productQuantity\": 1\n"
"        },\n"
"        \"4\": {\n"
"            \"price\": 19999,\n"
"            \"sellerName\": \"TechStore\",\n"
"            \"name\": \"Samsung Galaxy A54 5G 8/256 ГБ, черный\",\n"
"            \"rating\": 4.9,\n"
"            \"productQuantity\": 0\n"
"        },\n"
"        \"5\": {\n"
"            \"price\": 24999.58,\n"
"            \"sellerName\": \"Fzon\",\n"
"            \"name\": \"Apple iPhone 11 64 ГБ, черный\",\n"
"            \"rating\": 4.8,\n"
"            \"productQuantity\": 3\n"
"        },\n"
"        \"6\": {\n"
"            \"price\": 15999,\n"
"            \"sellerName\": \"GadgetStore\",\n"
"            \"name\": \"POCO X5 Pro 5G 8/256 ГБ, синий\",\n"
"            \"rating\": 4.6,\n"
"            \"productQuantity\": 0\n"
"        }\n"
"    }\n"
"}";
    } catch (const userver::formats::json::TypeMismatchException& e) {
        // Ошибка несоответствия типа данных
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return "{\"error\": \"Invalid field type\"}";
    } catch (const std::exception& e) {
        // Другие ошибки (например, ошибка базы данных)
        LOG_ERROR() << "Error while fetch products: " << e.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "{\"error\": \"Internal server error\"}";
    }
}

}  // namespace catalogservice

