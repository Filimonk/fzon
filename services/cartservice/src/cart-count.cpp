#include <cart-count.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/server/http/http_status.hpp>

namespace cartservice {

CartCount::CartCount(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string CartCount::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    const auto login_header = request.GetHeader("X-Login");
    if (login_header.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return "";
    }

    return "{\"cartCount\": 55}";
}

}  // namespace cartservice
