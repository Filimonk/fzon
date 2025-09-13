#pragma once

#include <userver/server/handlers/http_handler_base.hpp>

namespace authservice {

class Authentication final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-authentication";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const userver::server::http::HttpRequest&, userver::server::request::RequestContext&)
        const override;
};

}  // namespace authservice
