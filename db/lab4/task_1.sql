-- Количество ремонтов по производителям

SELECT m.name, COUNT(r.repair_id) as repair_count
FROM manufacturers m
JOIN products p ON m.manufacturer_id = p.manufacturer_id
JOIN repairs r ON p.product_id = r.product_id
GROUP BY m.name
ORDER BY repair_count DESC;

-- Мастерские с наибольшим количеством выполненных ремонтов
SELECT s.address, c.name as city, COUNT(r.repair_id) as completed_repairs
FROM repair_shops s
JOIN cities c ON s.city_id = c.city_id
JOIN repairs r ON s.shop_id = r.shop_id
WHERE r.end_date IS NOT NULL
GROUP BY s.shop_id, s.address, c.name
ORDER BY completed_repairs DESC
LIMIT 10;

-- Средняя стоимость ремонта по категориям товаров
SELECT p.category, 
       AVG(r.actual_repair_cost) as avg_repair_cost,
       AVG(r.client_cost) as avg_client_cost
FROM products p
JOIN repairs r ON p.product_id = r.product_id
GROUP BY p.category
ORDER BY avg_repair_cost DESC;