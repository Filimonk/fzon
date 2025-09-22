\connect fzon

CREATE SEQUENCE catalogserviceschema.product_article_seq START 1;

CREATE TABLE IF NOT EXISTS catalogserviceschema.products (
    article VARCHAR(20) PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    price DECIMAL(10, 2) NOT NULL,
    description TEXT NOT NULL,
    seller_name VARCHAR(255) NOT NULL,
    rating DECIMAL(2, 1) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
