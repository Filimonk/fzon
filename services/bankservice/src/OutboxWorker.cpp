#include "OutboxWorker.hpp"

#include <userver/components/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/storages/postgres/component.hpp>

#include <userver/formats/json.hpp>
#include <userver/utils/async.hpp>

#include <chrono>
#include <thread>
#include <random>

namespace bankservice {

OutboxWorker::OutboxWorker(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : components::ComponentBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient())
{
    std::srand(std::time(nullptr));
    utils::PeriodicTask::Settings settings{std::chrono::seconds{1}};
    periodic_task_.Start("bank-outbox-task", settings, [this] { DoWork(); });
}

OutboxWorker::~OutboxWorker() {
    periodic_task_.Stop();
}

void OutboxWorker::DoWork() {
    try {
        auto transaction = pg_cluster_->Begin(
            userver::storages::postgres::ClusterHostType::kMaster,
            userver::storages::postgres::TransactionOptions{}
        );

        auto result = transaction.Execute(
            "SELECT id, user_id, payload FROM outbox "
            "WHERE processed = false ORDER BY id FOR UPDATE SKIP LOCKED LIMIT 10"
        );

        if (result.Size() == 0) {
            transaction.Rollback();
            return;
        }

        for (const auto& row : result) {
            const auto outbox_id = row["id"].As<int>();
            const auto user_id   = row["user_id"].As<int>();
            const auto payload   = row["payload"].As<userver::formats::json::Value>();

            try {
                transaction.Execute("SAVEPOINT outbox_record_" + std::to_string(outbox_id));

                const auto order_id = payload["order_id"].As<int>();
                const auto amount   = payload["amount"].As<double>();

                std::this_thread::sleep_for(std::chrono::seconds(9)); // имитация долгой обработки

                auto user_res = transaction.Execute(
                    "SELECT balance::float8 FROM users WHERE user_id = $1",
                    user_id
                );

                std::string status = "INSUFFICIENT_FUNDS";

                if (user_res.Size() > 0) {
                    double balance = user_res[0]["balance"].As<double>();
                    if (balance >= amount) {
                        if (std::rand() % 2 == 0) {
                            transaction.Execute(
                                "UPDATE users SET balance = balance - $1 WHERE user_id = $2",
                                amount, user_id
                            );

                            status = "PAID";
                        }
                        else {
                            status = "FAILED";
                        }
                    }
                }

                transaction.Execute(
                    "INSERT INTO payments (order_id, user_id, amount, status) "
                    "VALUES ($1, $2, $3, $4)",
                    order_id, user_id, amount, status
                );

                transaction.Execute(
                    "UPDATE outbox SET processed = true WHERE id = $1",
                    outbox_id
                );

                // уведомляем orderservice
                // TODO: вынести в отдельный таск-процессор
                userver::formats::json::ValueBuilder res_json;
                res_json["order_id"] = order_id;
                res_json["status"]   = status;

                try {
                    auto response = http_client_.CreateRequest()
                        .post()
                        .url("http://orderservice:8080/payment-result")
                        .data(userver::formats::json::ToString(res_json.ExtractValue()))
                        .timeout(std::chrono::seconds(5))
                        .perform();

                    if (response->status_code() != 204) {
                        LOG_INFO() << "Sent payment result to orderservice successfuly";
                    } else {
                        LOG_WARNING() << "Payment result service returned status: "
                                      << response->status_code();
                    }

                } catch (const std::exception& ex) {
                    LOG_ERROR() << "Failed to send payment result: " << ex.what();
                }

            } catch (const std::exception& ex) {
                LOG_ERROR() << "Error processing bank outbox record " << outbox_id << ": " << ex.what();
                transaction.Execute("ROLLBACK TO SAVEPOINT outbox_record_" + std::to_string(outbox_id));
            }
        }

        transaction.Commit();

    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in OutboxWorker DoWork: " << ex.what();
    }
}

yaml_config::Schema OutboxWorker::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: Bank OutboxWorker component
additionalProperties: false
properties: {}
)");
}

}  // namespace bankservice

