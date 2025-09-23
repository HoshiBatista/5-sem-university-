
--  Добавление городов
INSERT INTO cities (name) VALUES
    ('Москва'),
    ('Санкт-Петербург'),
    ('Новосибирск');

-- Добавление производителей
INSERT INTO manufacturers (name, contact_info) VALUES
    ('Xiaomi', 'Китай, Пекин, ул. Центральная, 1. Тел.: +86-10-1234567'),
    ('Samsung', 'Южная Корея, Сувон. Тел.: +82-31-200-1111'),
    ('Bosch', 'Германия, Штутгарт. Тел.: +49-711-811-0');

-- Добавление товаров
INSERT INTO products (manufacturer_id, model_name, category, warranty_period, base_repair_cost) VALUES
    (1, 'Redmi Note 13', 'смартфоны', 365, 5000.00),
    (2, 'Galaxy S24', 'смартфоны', 730, 15000.00),
    (3, 'KGN39VI32R', 'холодильники', 1095, 12000.00),
    (1, 'Robot Vacuum X10', 'пылесосы', 180, 8000.00);

--  Добавление мастерских
INSERT INTO repair_shops (city_id, address, contact_phone) VALUES
    (1, 'ул. Тверская, 25', '+7-495-111-22-33'),
    (1, 'пр-т Мира, 100', '+7-495-444-55-66'),
    (2, 'Невский пр-т, 50', '+7-812-777-88-99'),
    (3, 'ул. Ленина, 1', '+7-383-123-45-67');

-- Добавление специализаций мастерских
INSERT INTO shop_specializations (shop_id, product_id) VALUES
    (1, 1), -- Мастерская 1 ремонтирует Xiaomi smartphones
    (1, 2), -- Мастерская 1 ремонтирует Samsung smartphones
    (2, 3), -- Мастерская 2 ремонтирует холодильники
    (3, 1), -- Мастерская 3 ремонтирует Xiaomi smartphones
    (3, 4), -- Мастерская 3 ремонтирует пылесосы
    (4, 2), -- Мастерская 4 ремонтирует Samsung smartphones
    (4, 3); -- Мастерская 4 ремонтирует холодильники

-- Добавление записей о ремонтах
INSERT INTO repairs (product_id, shop_id, start_date, end_date, actual_repair_cost, client_cost, serial_number, defect_description) VALUES
    (1, 1, '2024-01-15', '2024-01-20', 3000.00, 0.00, 'XN13123456', 'Разбит экран, гарантийный случай'),
    (2, 1, '2024-02-01', '2024-02-05', 0.00, 12000.00, 'GS24123456', 'Замена аккумулятора, не гарантия'),
    (3, 2, '2024-02-10', NULL, 8500.00, 0.00, 'BOSCH98765', 'Утечка фреона, гарантийный ремонт'),
    (4, 3, '2024-01-20', '2024-01-25', 5000.00, 5000.00, 'X10054321', 'Не работает двигатель, постгарантия');

-- Изменение контактного телефона мастерской с shop_id = 2
UPDATE repair_shops
SET contact_phone = '+7-495-999-00-11'
WHERE shop_id = 2;

-- Увеличение базовой стоимости ремонта для всех смартфонов на 5%
UPDATE products
SET base_repair_cost = base_repair_cost * 1.05
WHERE category = 'смартфоны';

-- Завершение ремонта, у которого не была указана end_date
UPDATE repairs
SET end_date = '2024-02-15'
WHERE repair_id = 3;

-- Удаление специализации (например, мастерская 4 больше не ремонтирует холодильники)
DELETE FROM shop_specializations
WHERE shop_id = 4 AND product_id = 3;

-- Удаление записи о ремонте (если она была создана ошибочно)
DELETE FROM repairs
WHERE repair_id = 4;
