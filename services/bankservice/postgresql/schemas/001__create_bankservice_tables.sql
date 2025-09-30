\connect fzon

-- Пользователи банка
CREATE TABLE IF NOT EXISTS bankserviceschema.users (
    user_id SERIAL PRIMARY KEY, -- FORIEGIN KEY на таблицу юзеров из auth
    balance NUMERIC(12,2) NOT NULL DEFAULT 0.0
);

-- Платежи
CREATE TABLE IF NOT EXISTS bankserviceschema.payments (
    id SERIAL PRIMARY KEY,
    order_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    amount NUMERIC(10,2) NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    created_at TIMESTAMP DEFAULT now()
);

-- Outbox банка
CREATE TABLE IF NOT EXISTS bankserviceschema.outbox (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    payload JSONB NOT NULL,
    created_at TIMESTAMP DEFAULT now(),
    processed BOOLEAN DEFAULT false
);

