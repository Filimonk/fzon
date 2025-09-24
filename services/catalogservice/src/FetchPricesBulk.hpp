#pragma once

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/storages/postgres/cluster.hpp>

namespace catalogservice {

class FetchPricesBulk final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-fetch-prices-bulk";

    FetchPricesBulk(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    std::string HandleRequestThrow(const userver::server::http::HttpRequest&, userver::server::request::RequestContext&)
        const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    userver::clients::http::Client& http_client_;
};

}  // namespace catalogservice
