\connect fzon

CREATE TABLE IF NOT EXISTS cartserviceschema.cart (
    user_id INTEGER NOT NULL,
    article VARCHAR(20) NOT NULL,
    quantity INTEGER NOT NULL,
    PRIMARY KEY (user_id, article)
    --FOREIGN KEY (user_id) REFERENCES authserviceschema.users(id) ON DELETE CASCADE,
    --FOREIGN KEY (article) REFERENCES catalogserviceschema.products(id) ON DELETE CASCADE
);
