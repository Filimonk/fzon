#include <PaymentResult.hpp>

#include <userver/storages/postgres/component.hpp>

namespace orderservice {

PaymentResult::PaymentResult(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string PaymentResult::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    try {
        const auto body_json = userver::formats::json::FromString(request.RequestBody());
        int order_id = body_json["order_id"].As<int>();
        std::string status = body_json["status"].As<std::string>();

        if (status == "SUCCESS") {
            pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "UPDATE orders SET status = 'PAID' WHERE id = $1",
                order_id
            );
        } else {
            pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "UPDATE orders SET status = 'FAILED' WHERE id = $1",
                order_id
            );
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in payment-result: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return R"({"error": "Internal server error"})";
    }
}

}  // namespace orderservice
