\connect fzon

INSERT INTO catalogserviceschema.products (article, name, price, description, seller_name, rating) VALUES
('0001', 'Tecno Смартфон SPARK 30C 6/128 ГБ, черный', 8990.00, 'Смартфон Tecno SPARK 30C с объемом памяти 6/128 ГБ, черный цвет', 'GadgetStore', 4.9),
('0002', 'Xiaomi Redmi Note 12 6/128 ГБ, синий', 18990.00, 'Смартфон Xiaomi Redmi Note 12 с объемом памяти 6/128 ГБ, синий цвет', 'Fzon', 4.7),
('0003', 'Realme C55 6/128 ГБ, солнечный желтый', 14990.00, 'Смартфон Realme C55 с объемом памяти 6/128 ГБ, солнечный желтый цвет', 'TechStore', 4.8),
('0004', 'Samsung Galaxy A54 5G 8/256 ГБ, черный', 34990.00, 'Смартфон Samsung Galaxy A54 5G с объемом памяти 8/256 ГБ, черный цвет', 'TechStore', 4.9),
('0005', 'Apple iPhone 11 64 ГБ, черный', 42990.00, 'Смартфон Apple iPhone 11 с объемом памяти 64 ГБ, черный цвет', 'Fzon', 4.8),
('0006', 'POCO X5 Pro 5G 8/256 ГБ, синий', 27990.00, 'Смартфон POCO X5 Pro 5G с объемом памяти 8/256 ГБ, синий цвет', 'GadgetStore', 4.6),
('0007', 'Samsung Galaxy S23 Ultra 12/512 ГБ, зеленый', 109990.00, 'Флагманский смартфон Samsung Galaxy S23 Ultra с S-Pen', 'PremiumTech', 4.9),
('0008', 'Apple iPhone 15 Pro 128 ГБ, титановый синий', 124990.00, 'Флагманский смартфон Apple iPhone 15 Pro', 'iStore', 4.9),
('0009', 'Google Pixel 8 Pro 12/256 ГБ, черный', 89990.00, 'Смартфон Google Pixel 8 Pro с продвинутой камерой', 'TechStore', 4.8),
('0010', 'OnePlus 11 16/256 ГБ, зеленый', 64990.00, 'Смартфон OnePlus 11 с быстрой зарядкой', 'GadgetStore', 4.7),
('0011', 'Xiaomi 13T Pro 12/512 ГБ, синий', 59990.00, 'Смартфон Xiaomi 13T Pro с камерой Leica', 'Fzon', 4.8),
('0012', 'Nothing Phone (2) 12/256 ГБ, белый', 49990.00, 'Смартфон Nothing Phone (2) с уникальной подсветкой', 'TechStore', 4.6);

-- Двигаем последовательность
SELECT SETVAL('catalogserviceschema.product_article_seq', 12);

