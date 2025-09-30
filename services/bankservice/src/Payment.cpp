#include "Payment.hpp"

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>

namespace bankservice {

Payment::Payment(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string Payment::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const
{
    try {
        const auto body_json = userver::formats::json::FromString(request.RequestBody());

        const auto order_id = body_json["order_id"].As<int>();
        const auto user_id  = body_json["user_id"].As<int>();
        const auto amount   = body_json["amount"].As<double>();

        // сохраняем в outbox
        pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO outbox (user_id, payload) VALUES ($1, $2::jsonb)",
            user_id,
            body_json
        );

        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while enqueuing payment: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return R"({"error": "internal server error"})";
    }
}

}  // namespace bankservice

