#include "OutboxWorker.hpp"

#include <userver/components/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/storages/postgres/component.hpp>

#include <userver/formats/json.hpp>
#include <userver/utils/async.hpp>

#include <chrono>

namespace orderservice {
    
OutboxWorker::OutboxWorker(const components::ComponentConfig& config,
                           const components::ComponentContext& component_context)
    : components::ComponentBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()),
      http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient())
{
    std::chrono::seconds period{1};

    utils::PeriodicTask::Settings settings{period};

    periodic_task_.Start(
        "outbox-worker-task",
        settings,
        [this] { DoWork(); }
    );
}

OutboxWorker::~OutboxWorker() {
    // Останавливаем периодическую задачу перед уничтожением объекта
    periodic_task_.Stop();
}

void OutboxWorker::DoWork() {
    try {
        auto transaction = pg_cluster_->Begin(
            userver::storages::postgres::ClusterHostType::kMaster,
            userver::storages::postgres::TransactionOptions{}
        );
        
        // Блокируем и выбираем непроцессированные записи с помощью FOR UPDATE SKIP LOCKED
        auto result = transaction.Execute(
            "SELECT id, user_id, payload FROM outbox WHERE processed = false "
            "ORDER BY id FOR UPDATE SKIP LOCKED LIMIT 10"  // Ограничиваем batch размер
        );

        if (result.Size() == 0) {
            transaction.Rollback();
            return; // Нет записей для обработки
        }

        // Обрабатываем каждую запись
        for (const auto& row : result) {
            const auto outbox_id = row["id"].As<int>();
            const auto user_id = row["user_id"].As<int>();
            const auto payload_json = row["payload"].As<userver::formats::json::Value>();

            try {
                // Создаем точку сохранения для этой записи
                transaction.Execute("SAVEPOINT outbox_record_" + std::to_string(outbox_id));
                
                // Парсим payload
                const auto items = payload_json["cart_items"];

                // Вычисляем total_amount и готовим данные для вставки
                double total_amount = 0.0;
                std::vector<std::tuple<std::string, int, double>> order_items_data;

                for (const auto& item : items) {
                    const auto article = item["article"].As<std::string>();
                    const auto quantity = item["quantity"].As<int>();
                    const auto price = item["price"].As<double>();

                    total_amount += quantity * price;
                    order_items_data.emplace_back(article, quantity, price);
                }
    
                // Создаем заказ
                auto order_result = transaction.Execute(
                    "INSERT INTO orders (user_id, total_amount, status) "
                    "VALUES ($1, $2, 'PENDING') RETURNING id",
                    user_id, total_amount
                );

                const auto order_id = order_result[0]["id"].As<int>();

                // Добавляем элементы заказа
                for (const auto& item_data : order_items_data) {
                    const auto& article = std::get<0>(item_data);
                    const auto quantity = std::get<1>(item_data);
                    const auto price = std::get<2>(item_data);

                    transaction.Execute(
                        "INSERT INTO order_items (order_id, article, quantity, price) "
                        "VALUES ($1, $2, $3, $4)",
                        order_id, article, quantity, price
                    );
                }

                // Помечаем outbox запись как обработанную
                transaction.Execute(
                    "UPDATE outbox SET processed = true WHERE id = $1",
                    outbox_id
                );
    
                // Отправляем запрос в банковский сервис (асинхронно, не ждем ответ)
                // TODO: надо научиться слать его в новой корутине на отдельном таск процессоре
                userver::formats::json::ValueBuilder payment_request;
                payment_request["order_id"] = order_id;
                payment_request["user_id"] = user_id;
                payment_request["amount"] = total_amount;
                
                try {
                    auto response = http_client_.CreateRequest()
                        .post()
                        .url("http://bankservice:8080/payment")
                        .data(userver::formats::json::ToString(payment_request.ExtractValue()))
                        .timeout(std::chrono::seconds(30)) // Долгий таймаут для банковского сервиса
                        .perform();

                    if (response->status_code() != 204) {
                        LOG_INFO() << "Sent payment request to bankservice successfuly";
                    }
                    else {
                        LOG_WARNING() << "Payment service returned status: " << response->status_code();
                    }
                } catch (const std::exception& ex) {
                    LOG_ERROR() << "Failed to send payment request: " << ex.what();
                }

            } catch (const std::exception& ex) {
                // Откатываем только ЭТУ запись outbox, продолжаем остальные
                LOG_ERROR() << "Failed to process outbox record " << outbox_id << ": " << ex.what();
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
description: OutboxWorker component
additionalProperties: false
properties: {}
)");
}

}  // namespace orderservice

