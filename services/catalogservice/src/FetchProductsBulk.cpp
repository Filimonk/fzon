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
        return "";
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

