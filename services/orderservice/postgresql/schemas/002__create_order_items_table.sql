\connect fzon

CREATE TABLE IF NOT EXISTS orderserviceschema.order_items (
    id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES orderserviceschema.orders(id) ON DELETE CASCADE,
    article VARCHAR(20) NOT NULL,
    quantity INTEGER NOT NULL,
    price DECIMAL(10, 2) NOT NULL
);

