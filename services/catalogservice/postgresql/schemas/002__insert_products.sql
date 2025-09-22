\connect fzon

INSERT INTO catalogserviceschema.products (article, name, price, description, seller_name, rating) VALUES
('0001', 'Tecno Смартфон SPARK 30C 6/128 ГБ, черный', 5959.00, 'Смартфон Tecno SPARK 30C с объемом памяти 6/128 ГБ, черный цвет', 'GadgetStore', 4.9),
('0002', 'Xiaomi Redmi Note 12 6/128 ГБ, синий', 12999.00, 'Смартфон Xiaomi Redmi Note 12 с объемом памяти 6/128 ГБ, синий цвет', 'Fzon', 4.7),
('0003', 'Realme C55 6/128 ГБ, солнечный желтый', 8999.00, 'Смартфон Realme C55 с объемом памяти 6/128 ГБ, солнечный желтый цвет', 'TechStore', 4.8),
('0004', 'Samsung Galaxy A54 5G 8/256 ГБ, черный', 19999.00, 'Смартфон Samsung Galaxy A54 5G с объемом памяти 8/256 ГБ, черный цвет', 'TechStore', 4.9),
('0005', 'Apple iPhone 11 64 ГБ, черный', 24999.00, 'Смартфон Apple iPhone 11 с объемом памяти 64 ГБ, черный цвет', 'Fzon', 4.8),
('0006', 'POCO X5 Pro 5G 8/256 ГБ, синий', 15999.00, 'Смартфон POCO X5 Pro 5G с объемом памяти 8/256 ГБ, синий цвет', 'GadgetStore', 4.6);

-- Двигаем последовательность
SELECT SETVAL('catalogserviceschema.product_article_seq', 6);

