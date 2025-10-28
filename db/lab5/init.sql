CREATE TABLE IF NOT EXISTS manufacturers (
    manufacturer_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    contact_info TEXT
);

CREATE TABLE IF NOT EXISTS cities (
    city_id SERIAL PRIMARY KEY,
    name VARCHAR(50) NOT NULL
);

CREATE TABLE IF NOT EXISTS products (
    product_id SERIAL PRIMARY KEY,
    manufacturer_id INTEGER NOT NULL,
    model_name VARCHAR(100) NOT NULL,
    category VARCHAR(50) NOT NULL,
    warranty_period INTEGER NOT NULL CHECK (warranty_period > 0),
    base_repair_cost DECIMAL(10,2) NOT NULL CHECK (base_repair_cost >= 0),
    CONSTRAINT fk_products_manufacturer
        FOREIGN KEY (manufacturer_id)
        REFERENCES manufacturers(manufacturer_id)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS repair_shops (
    shop_id SERIAL PRIMARY KEY,
    city_id INTEGER NOT NULL,
    address VARCHAR(255) NOT NULL,
    contact_phone VARCHAR(20),
    CONSTRAINT fk_repair_shops_city
        FOREIGN KEY (city_id)
        REFERENCES cities(city_id)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS shop_specializations (
    specialization_id SERIAL PRIMARY KEY,
    shop_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    CONSTRAINT uq_shop_product UNIQUE (shop_id, product_id),
    CONSTRAINT fk_specializations_shop
        FOREIGN KEY (shop_id)
        REFERENCES repair_shops(shop_id)
        ON DELETE CASCADE
        ON UPDATE CASCADE,
    CONSTRAINT fk_specializations_product
        FOREIGN KEY (product_id)
        REFERENCES products(product_id)
        ON DELETE CASCADE
        ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS repairs (
    repair_id SERIAL PRIMARY KEY,
    product_id INTEGER NOT NULL,
    shop_id INTEGER NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE,
    actual_repair_cost DECIMAL(10,2) NOT NULL CHECK (actual_repair_cost >= 0),
    client_cost DECIMAL(10,2) NOT NULL CHECK (client_cost >= 0),
    serial_number VARCHAR(50),
    defect_description TEXT,
    CONSTRAINT chk_repair_dates CHECK (end_date IS NULL OR end_date >= start_date),
    CONSTRAINT fk_repairs_product
        FOREIGN KEY (product_id)
        REFERENCES products(product_id)
        ON DELETE RESTRICT
        ON UPDATE CASCADE,
    CONSTRAINT fk_repairs_shop
        FOREIGN KEY (shop_id)
        REFERENCES repair_shops(shop_id)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
);

INSERT INTO cities (name) VALUES 
    ('Москва'),
    ('Санкт-Петербург'),
    ('Новосибирск'),
    ('Екатеринбург'),
    ('Казань');

INSERT INTO manufacturers (name, contact_info) VALUES 
    ('Xiaomi', 'Китай, Пекин, ул. Центральная, 1. Тел.: +86-10-1234567'),
    ('Samsung', 'Южная Корея, Сувон. Тел.: +82-31-200-1111'),
    ('Bosch', 'Германия, Штутгарт. Тел.: +49-711-811-0'),
    ('LG', 'Южная Корея, Сеул. Тел.: +82-2-3777-1111'),
    ('Apple', 'США, Купертино. Тел.: +1-408-996-1010');

INSERT INTO products (manufacturer_id, model_name, category, warranty_period, base_repair_cost) VALUES 
    (1, 'Redmi Note 13', 'смартфоны', 365, 5000.00),
    (2, 'Galaxy S24', 'смартфоны', 730, 15000.00),
    (3, 'KGN39VI32R', 'холодильники', 1095, 12000.00),
    (1, 'Robot Vacuum X10', 'пылесосы', 180, 8000.00),
    (4, 'OLED55C3', 'телевизоры', 730, 20000.00),
    (5, 'iPhone 15 Pro', 'смартфоны', 365, 25000.00),
    (3, 'WTG86401RU', 'стиральные машины', 1095, 15000.00);

INSERT INTO repair_shops (city_id, address, contact_phone) VALUES 
    (1, 'ул. Тверская, 25', '+7-495-111-22-33'),
    (1, 'пр-т Мира, 100', '+7-495-444-55-66'),
    (2, 'Невский пр-т, 50', '+7-812-777-88-99'),
    (3, 'ул. Ленина, 1', '+7-383-123-45-67'),
    (4, 'ул. Малышева, 45', '+7-343-555-66-77'),
    (5, 'ул. Баумана, 15', '+7-843-999-00-11');

INSERT INTO shop_specializations (shop_id, product_id) VALUES 
    (1, 1), (1, 2), (1, 6),
    (2, 3), (2, 7),
    (3, 1), (3, 4),
    (4, 2), (4, 3),
    (5, 5), (5, 6),
    (6, 1), (6, 2), (6, 6);

INSERT INTO repairs (product_id, shop_id, start_date, end_date, actual_repair_cost, client_cost, serial_number, defect_description) VALUES 
    (1, 1, '2024-01-15', '2024-01-20', 3000.00, 0.00, 'XN13123456', 'Разбит экран, гарантийный случай'),
    (2, 1, '2024-02-01', '2024-02-05', 0.00, 12000.00, 'GS24123456', 'Замена аккумулятора, не гарантия'),
    (3, 2, '2024-02-10', '2024-02-15', 8500.00, 0.00, 'BOSCH98765', 'Утечка фреона, гарантийный ремонт'),
    (4, 3, '2024-01-20', '2024-01-25', 5000.00, 5000.00, 'X10054321', 'Не работает двигатель, постгарантия'),
    (6, 5, '2024-03-01', NULL, 15000.00, 0.00, 'IP15P88888', 'Не работает камера, гарантийный'),
    (2, 6, '2024-02-20', '2024-02-25', 2000.00, 2000.00, 'GS24987654', 'Замена разъема зарядки');