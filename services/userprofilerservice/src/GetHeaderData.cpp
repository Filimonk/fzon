#include "GetHeaderData.hpp"

#include <userver/http/common_headers.hpp>
#include <userver/formats/json.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/server/http/http_status.hpp>

namespace userprofilerservice {

GetHeaderData::GetHeaderData(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      http_client_(context.FindComponent<userver::components::HttpClient>()
                       .GetHttpClient()) {}

std::string GetHeaderData::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  // Проверяем наличие заголовка Authorization
  const auto auth_header = request.GetHeader("Authorization");
  if (auth_header.empty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
    return "";
  }

  try {
    // Формируем запрос к authservice
    auto response = http_client_.CreateRequest()
        .get()
        .url("http://authservice:8080/authentication/")
        .headers({{"Authorization", auth_header}})
        .timeout(std::chrono::seconds(2))
        .perform();

    // Проверяем статус ответа (используем числовое значение 200 вместо перечисления)
    if (response->status_code() != 200) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
      return "";
    }

    // Парсим JSON ответ
    const auto json_body = userver::formats::json::FromString(response->body());
    const auto status = json_body["status"].As<std::string>();

    if (status == "approved") {
      return "{\"cartCount\": 55, \"username\": \"Filimon\"}";
    } else {
      request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
      return "";
    }
  } catch (const std::exception& ex) {
    // Логируем ошибку
    LOG_ERROR() << "Error while requesting auth service: " << ex.what();
    request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
    return "";
  }
}

}  // namespace userprofilerservice
