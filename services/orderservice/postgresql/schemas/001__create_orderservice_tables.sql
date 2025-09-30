\connect fzon

CREATE TABLE IF NOT EXISTS orderserviceschema.orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    total_amount NUMERIC(10,2) NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    created_at TIMESTAMP DEFAULT now()
);

CREATE TABLE IF NOT EXISTS orderserviceschema.order_items (
    order_id INTEGER NOT NULL REFERENCES orderserviceschema.orders(id) ON DELETE CASCADE,
    article VARCHAR(20) NOT NULL,
    quantity INTEGER NOT NULL,
    price NUMERIC(10,2) NOT NULL,
    PRIMARY KEY (order_id, article)
);

CREATE TABLE IF NOT EXISTS orderserviceschema.outbox (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    payload JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT now(),
    processed BOOLEAN DEFAULT false
);

