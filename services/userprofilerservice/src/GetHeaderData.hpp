#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/components/component_context.hpp>

namespace userprofilerservice {

class GetHeaderData final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-get-header-data";

  GetHeaderData(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  userver::clients::http::Client& http_client_;
};

}  // namespace userprofilerservice
