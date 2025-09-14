#include <registration.hpp>

#include <userver/storages/postgres/component.hpp>


namespace authservice {

Registration::Registration(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string
Registration::HandleRequestThrow(const userver::server::http::HttpRequest& request,
                                userver::server::request::RequestContext&) const {
    return "{\"token\": \"JSON\"}";
}

}  // namespace authservice
