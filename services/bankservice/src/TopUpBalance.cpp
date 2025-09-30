#include "TopUpBalance.hpp"

#include <userver/storages/postgres/component.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/formats/json.hpp>
#include <userver/clients/http/component.hpp>

namespace bankservice {

TopUpBalance::TopUpBalance(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string TopUpBalance::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    
    const auto auth_header = request.GetHeader("Authorization");
    if (auth_header.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return "";
    }

    try {
        auto response = http_client_.CreateRequest()
            .get()
            .url("http://authservice:8080/verify/")
            .headers({{"Authorization", auth_header}})
            .timeout(std::chrono::seconds(2))
            .perform();

        if (response->status_code() != 200) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return "";
        }

        const auto json_body = userver::formats::json::FromString(response->body());
        const auto user_id = json_body["user_id"].As<int>();

        const auto request_json = userver::formats::json::FromString(request.RequestBody());
        if (!request_json.HasMember("amount")) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "";
        }

        const auto amount = request_json["amount"].As<double>();
        if (amount <= 0) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return "";
        }

        pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO users (user_id, balance) "
            "VALUES ($1, $2) "
            "ON CONFLICT (user_id) "
            "DO UPDATE SET balance = users.balance + $2",
            user_id, amount
        );

        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while topping up balance: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "";
    }
}

}  // namespace bankservice

