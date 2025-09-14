#include <authentication.hpp>

namespace authservice {

std::string
Authentication::HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
    const {
    return "{\"token\": \"JWT\"}";
}

}  // namespace authservice
