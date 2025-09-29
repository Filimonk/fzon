#pragma once

#include <userver/utest/using_namespace_userver.hpp>
#include <userver/components/component_base.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/storages/postgres/cluster.hpp>


namespace orderservice {

class OutboxWorker final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "outbox-worker";

    OutboxWorker(const components::ComponentConfig& config,
                 const components::ComponentContext& component_context);

    ~OutboxWorker() final;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void DoWork();

    utils::PeriodicTask periodic_task_;
    userver::storages::postgres::ClusterPtr pg_cluster_;
    userver::clients::http::Client& http_client_;
};

}  // namespace orderservice

