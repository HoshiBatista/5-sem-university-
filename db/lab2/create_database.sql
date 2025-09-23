-- DROP DATABASE IF EXISTS repair_service;

CREATE DATABASE repair_service;

-- Таблица 'manufacturers'
CREATE TABLE manufacturers (
    manufacturer_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    contact_info TEXT
);

-- Таблица 'cities'
CREATE TABLE cities (
    city_id SERIAL PRIMARY KEY,
    name VARCHAR(50) NOT NULL
);

-- Таблица 'products'
CREATE TABLE products (
    product_id SERIAL PRIMARY KEY,
    manufacturer_id INTEGER NOT NULL,
    model_name VARCHAR(100) NOT NULL,
    category VARCHAR(50) NOT NULL,
    warranty_period INTEGER NOT NULL CHECK (warranty_period > 0),
    base_repair_cost DECIMAL(10,2) NOT NULL CHECK (base_repair_cost >= 0),

    CONSTRAINT fk_products_manufacturer
        FOREIGN KEY (manufacturer_id)
        REFERENCES manufacturers(manufacturer_id)
        ON DELETE RESTRICT   -- Запрещает удаление производителя, если есть его товары
        ON UPDATE CASCADE    -- Обновляет manufacturer_id в товарах при изменении в manufacturers
);

-- Таблица 'repair_shops'
CREATE TABLE repair_shops (
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

-- Таблица 'shop_specializations'
CREATE TABLE shop_specializations (
    specialization_id SERIAL PRIMARY KEY,
    shop_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,

    CONSTRAINT uq_shop_product UNIQUE (shop_id, product_id),
  
    CONSTRAINT fk_specializations_shop
        FOREIGN KEY (shop_id)
        REFERENCES repair_shops(shop_id)
        ON DELETE CASCADE   -- Удаляет специализацию при удалении мастерской
        ON UPDATE CASCADE,
  
    CONSTRAINT fk_specializations_product
        FOREIGN KEY (product_id)
        REFERENCES products(product_id)
        ON DELETE CASCADE   -- Удаляет специализацию при удалении товара
        ON UPDATE CASCADE
);

-- 6. Таблица 'repairs'
CREATE TABLE repairs (
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