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
      http_client_(context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string GetHeaderData::HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const {
    const auto auth_header = request.GetHeader("Authorization");
    if (auth_header.empty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
        return "";
    }

    try {
        // Запрос к сервису аутентификации
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

        // Парсим ответ от authservice
        const auto json_body = userver::formats::json::FromString(response->body());
        const auto login = json_body["login"].As<std::string>();
        const auto username = json_body["username"].As<std::string>();

        // Запрос к сервису корзины
        response = http_client_.CreateRequest()
            .get()
            .url("http://cartservice:8080/cart-count/")
            .headers({{"Authorization", auth_header}})
            .timeout(std::chrono::seconds(2))
            .perform();

        // Проверяем статус ответа от cartservice
        if (response->status_code() != 200) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return "";
        }

        // Парсим ответ от cartservice
        const auto cart_json = userver::formats::json::FromString(response->body());
        const auto cart_count = cart_json["cartCount"].As<int>();

        // Формируем итоговый JSON
        userver::formats::json::ValueBuilder result;
        result["username"] = username;
        result["cartCount"] = cart_count;

        // Устанавливаем правильный Content-Type
        request.GetHttpResponse().SetContentType("application/json");
        return userver::formats::json::ToString(result.ExtractValue());
    }
    catch (const std::exception& ex) {
        LOG_ERROR() << "Error while requesting external service: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return "";
    }
}

}  // namespace userprofilerservice

