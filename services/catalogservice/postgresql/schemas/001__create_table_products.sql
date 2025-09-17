\connect fzon

CREATE TABLE IF NOT EXISTS catalogserviceschema.products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    price DECIMAL(10, 2) NOT NULL,
    description TEXT,
    seller_name VARCHAR(255) NOT NULL,
    rating DECIMAL(2, 1),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
