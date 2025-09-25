\connect fzon

CREATE TABLE IF NOT EXISTS orderserviceschema.orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    total_amount DECIMAL(10, 2),
    status VARCHAR(50) NOT NULL DEFAULT 'payment_processing',
    idempotency_token VARCHAR(255) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

