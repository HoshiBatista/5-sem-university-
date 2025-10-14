WITH 
-- Рейтинг убыточности товаров
product_unprofitability AS (
    SELECT 
        p.product_id,
        p.model_name,
        m.name as manufacturer,
        p.category,
        COUNT(r.repair_id) as repair_count,
        SUM(r.actual_repair_cost) as total_repair_cost,
        RANK() OVER (ORDER BY COUNT(r.repair_id) DESC, SUM(r.actual_repair_cost) DESC) as unprofitability_rank
    FROM products p
    JOIN manufacturers m ON p.manufacturer_id = m.manufacturer_id
    LEFT JOIN repairs r ON p.product_id = r.product_id
    GROUP BY p.product_id, p.model_name, m.name, p.category
    HAVING COUNT(r.repair_id) > 0
),

-- Обеспеченность городов мастерскими
city_coverage AS (
    SELECT 
        c.city_id,
        c.name as city_name,
        p.category,
        COUNT(DISTINCT s.shop_id) as shop_count,
        COUNT(DISTINCT sp.specialization_id) as specialization_count,
        COUNT(r.repair_id) as completed_repairs,
        CASE 
            WHEN COUNT(DISTINCT s.shop_id) > 0 THEN 
                ROUND(100.0 * COUNT(DISTINCT sp.specialization_id) / 
                      (SELECT COUNT(*) FROM products WHERE category = p.category), 1)
            ELSE 0 
        END as coverage_percentage
    FROM cities c
    LEFT JOIN repair_shops s ON c.city_id = s.city_id
    LEFT JOIN shop_specializations sp ON s.shop_id = sp.shop_id
    LEFT JOIN products p ON sp.product_id = p.product_id
    LEFT JOIN repairs r ON s.shop_id = r.shop_id AND r.end_date IS NOT NULL
    GROUP BY c.city_id, c.name, p.category
),

-- Сводка по городам (общая обеспеченность)
city_summary AS (
    SELECT 
        c.city_id,
        c.name as city_name,
        COUNT(DISTINCT s.shop_id) as total_shops,
        COUNT(DISTINCT sp.product_id) as unique_products_covered,
        COUNT(r.repair_id) as total_repairs
    FROM cities c
    LEFT JOIN repair_shops s ON c.city_id = s.city_id
    LEFT JOIN shop_specializations sp ON s.shop_id = sp.shop_id
    LEFT JOIN repairs r ON s.shop_id = r.shop_id AND r.end_date IS NOT NULL
    GROUP BY c.city_id, c.name
)

-- Основной отчет
SELECT 
    'Рейтинг убыточности и обеспеченности' as report_type,
    
    -- Данные по убыточности товаров
    pu.model_name,
    pu.manufacturer,
    pu.category as product_category,
    pu.repair_count,
    pu.total_repair_cost,
    pu.unprofitability_rank,
    
    -- Данные по обеспеченности
    cc.city_name,
    cc.category as coverage_category,
    cc.shop_count,
    cc.specialization_count,
    cc.completed_repairs,
    cc.coverage_percentage,
    
    -- Сводные данные по городам
    cs.total_shops as city_total_shops,
    cs.unique_products_covered as city_unique_products,
    cs.total_repairs as city_total_repairs,

    -- Комплексные метрики
    CASE 
        WHEN pu.unprofitability_rank <= 5 THEN 'Высокая убыточность'
        WHEN pu.unprofitability_rank <= 10 THEN 'Средняя убыточность'
        ELSE 'Низкая убыточность'
    END as profitability_category,

    CASE 
        WHEN cc.coverage_percentage >= 80 THEN 'Отличная обеспеченность'
        WHEN cc.coverage_percentage >= 50 THEN 'Хорошая обеспеченность'
        WHEN cc.coverage_percentage >= 20 THEN 'Удовлетворительная обеспеченность'
        ELSE 'Недостаточная обеспеченность'
    END as coverage_quality

FROM product_unprofitability pu
CROSS JOIN city_coverage cc
LEFT JOIN city_summary cs ON cc.city_id = cs.city_id

-- Фильтруем для показа наиболее значимых данных
WHERE pu.unprofitability_rank <= 15 
  AND cc.shop_count > 0
  AND cc.coverage_percentage IS NOT NULL

ORDER BY 
    pu.unprofitability_rank ASC,
    cc.coverage_percentage DESC,
    cc.city_name,
    cc.category;