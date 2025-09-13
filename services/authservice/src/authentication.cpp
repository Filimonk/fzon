#include <authentication.hpp>

namespace authservice {

std::string
Authentication::HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
    const {
    return "{\"status\": \"approved\"}";
}

}  // namespace authservice
