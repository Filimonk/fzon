#include <AddProduct.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/http/http_status.hpp>

namespace catalogservice {

AddProduct::AddProduct(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string AddProduct::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    try {
        // Парсим JSON из тела запроса
        const auto request_json = userver::formats::json::FromString(request.RequestBody());

        if (!request_json.HasMember("name")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "{\"field\": \"name\", \"error\": \"Missing required fields\"}";
        }
        if (!request_json.HasMember("price")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "{\"field\": \"price\", \"error\": \"Missing required fields\"}";
        }
        if (!request_json.HasMember("sellerName")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "{\"field\": \"seller-name\", \"error\": \"Missing required fields\"}";
        }
        if (!request_json.HasMember("description")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "{\"field\": \"description\", \"error\": \"Missing required fields\"}";
        }
        if (!request_json.HasMember("rating")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "{\"field\": \"rating\", \"error\": \"Missing required fields\"}";
        }
        
        const auto name = request_json["name"].As<std::string>();
        const auto price = request_json["price"].As<double>();
        const auto description = request_json["description"].As<std::string>();
        const auto seller_name = request_json["sellerName"].As<std::string>();
        const auto rating = request_json["rating"].As<double>();

        // Вставляем товар в базу данных
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO products (article, name, price, description, seller_name, rating) "
            "VALUES (LPAD((NEXTVAL('product_article_seq'))::text, 4, '0'), $1, $2, $3, $4, $5)",
            name, price, description, seller_name, rating
        );

        // Успешный ответ - 204 OK без тела
        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const userver::formats::json::TypeMismatchException& e) {
        // Ошибка несоответствия типа данных
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return "{\"error\": \"Invalid field type\"}";
    } catch (const std::exception& e) {
        // Другие ошибки (например, ошибка базы данных)
        LOG_ERROR() << "Error while adding product: " << e.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "{\"error\": \"Internal server error\"}";
    }
}

}  // namespace catalogservice

