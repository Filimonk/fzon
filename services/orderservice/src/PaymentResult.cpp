#include "PaymentResult.hpp"

#include <userver/storages/postgres/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/clients/http/component.hpp>

namespace orderservice {

PaymentResult::PaymentResult(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {}

std::string PaymentResult::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const
{
    try {
        const auto body_json = userver::formats::json::FromString(request.RequestBody());
        const auto order_id  = body_json["order_id"].As<int>();
        const auto status    = body_json["status"].As<std::string>();

        // Обновляем статус заказа
        pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "UPDATE orderserviceschema.orders SET status = $1 WHERE id = $2",
            status,
            order_id
        );

        // Если статус не "PAID", возвращаем товары в корзину
        if (status != "PAID") {
            // Получаем user_id и товары из заказа
            auto order_result = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "SELECT user_id FROM orderserviceschema.orders WHERE id = $1",
                order_id
            );

            if (order_result.Size() == 0) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
                return R"({"error":"order not found"})";
            }

            const auto user_id = order_result[0]["user_id"].As<int>();

            // Получаем все товары из заказа
            auto items_result = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "SELECT article, quantity FROM order_items WHERE order_id = $1",
                order_id
            );

            // Для каждого товара вызываем ручку корзины
            for (const auto& row : items_result) {
                const auto article = row["article"].As<std::string>();
                const auto quantity = row["quantity"].As<int>();

                try {
                    // Вызываем ручку cartservice для добавления товара в корзину
                    auto cart_response = http_client_.CreateRequest()
                        .post()
                        .url("http://cartservice:8080/change-cart-product-count-by-user-id")
                        .headers({{"Content-Type", "application/json"}})
                        .data(userver::formats::json::ToString(
                            userver::formats::json::MakeObject(
                                "userId", user_id,
                                "article", article,
                                "productQuantity", quantity
                            )
                        ))
                        .timeout(std::chrono::seconds(2))
                        .perform();

                    if (cart_response->status_code() != 204) {
                        LOG_ERROR() << "Failed to add item to cart: " << cart_response->body()
                                  << ", status: " << cart_response->status_code();
                    }

                } catch (const std::exception& ex) {
                    LOG_ERROR() << "Error while calling cartservice for article " << article
                              << ": " << ex.what();
                }
            }
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kNoContent);
        return "";

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in PaymentResult: " << ex.what();
        request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
        return R"({"error":"internal error"})";
    }
}

}  // namespace orderservice

