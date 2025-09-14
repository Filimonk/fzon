#include <verify.hpp>

namespace authservice {

std::string
Verify::HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
    const {
    return "{\"login\": \"Filimon\", \"username\": \"Филимон\"}";
}

}  // namespace authservice
