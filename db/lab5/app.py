import psycopg2
from psycopg2 import sql
import sys
from datetime import datetime
from textwrap import shorten

class RepairServiceDB:
    def __init__(self):
        self.connection = None
        self.connect()
    
    def connect(self):
        try:
            self.connection = psycopg2.connect(
                dbname="repair_service",
                user="postgres",
                password="password",
                host="localhost",
                port="5432"
            )
            print("✅ Успешное подключение к базе данных")
        except Exception as e:
            print(f"❌ Ошибка подключения к базе данных: {e}")
            sys.exit(1)
    
    def execute_query(self, query, params=None, fetch=False):
        try:
            with self.connection.cursor() as cursor:
                cursor.execute(query, params)
                if fetch:
                    columns = [desc[0] for desc in cursor.description]
                    results = cursor.fetchall()
                    return True, (columns, results)
                self.connection.commit()
                return True, None
        except Exception as e:
            print(f"❌ Ошибка выполнения запроса: {e}")
            self.connection.rollback()
            return False, str(e)
    
    def get_unprofitability_rating(self):
        query = """
        SELECT 
            p.model_name,
            m.name as manufacturer,
            p.category,
            COUNT(r.repair_id) as repair_count,
            SUM(r.actual_repair_cost) as total_repair_cost,
            ROUND(AVG(r.actual_repair_cost), 2) as avg_repair_cost,
            SUM(CASE WHEN r.client_cost = 0 THEN r.actual_repair_cost ELSE 0 END) as warranty_costs,
            RANK() OVER (ORDER BY COUNT(r.repair_id) DESC, SUM(r.actual_repair_cost) DESC) as problem_rank
        FROM products p
        JOIN manufacturers m ON p.manufacturer_id = m.manufacturer_id
        LEFT JOIN repairs r ON p.product_id = r.product_id
        WHERE r.repair_id IS NOT NULL
        GROUP BY p.product_id, p.model_name, m.name, p.category
        ORDER BY repair_count DESC, total_repair_cost DESC
        LIMIT 15;
        """
        return self.execute_query(query, fetch=True)
    
    def get_city_coverage_report(self):
        query = """
        WITH city_shop_stats AS (
            SELECT 
                c.city_id,
                c.name as city_name,
                COUNT(DISTINCT s.shop_id) as total_shops
            FROM cities c
            LEFT JOIN repair_shops s ON c.city_id = s.city_id
            GROUP BY c.city_id, c.name
        ),
        category_coverage AS (
            SELECT 
                c.city_id,
                p.category,
                COUNT(DISTINCT s.shop_id) as shops_with_category,
                COUNT(r.repair_id) as repairs_count
            FROM cities c
            LEFT JOIN repair_shops s ON c.city_id = s.city_id
            LEFT JOIN shop_specializations sp ON s.shop_id = sp.shop_id
            LEFT JOIN products p ON sp.product_id = p.product_id
            LEFT JOIN repairs r ON s.shop_id = r.shop_id AND r.product_id = p.product_id
            GROUP BY c.city_id, p.category
        )
        SELECT 
            css.city_name,
            COALESCE(cc.category, 'Все категории') as category,
            css.total_shops,
            COALESCE(cc.shops_with_category, 0) as specialized_shops,
            COALESCE(cc.repairs_count, 0) as repairs_done,
            CASE 
                WHEN css.total_shops > 0 THEN 
                    ROUND(100.0 * COALESCE(cc.shops_with_category, 0) / css.total_shops, 1)
                ELSE 0 
            END as coverage_percentage
        FROM city_shop_stats css
        LEFT JOIN category_coverage cc ON css.city_id = cc.city_id
        ORDER BY css.city_name, cc.category NULLS FIRST;
        """
        return self.execute_query(query, fetch=True)
    
    def add_repair(self, product_id, shop_id, serial_number, defect_description, is_warranty):

        cost_query = "SELECT base_repair_cost FROM products WHERE product_id = %s"
        success, result = self.execute_query(cost_query, (product_id,), fetch=True)
        
        if not success:
            return False, "Не удалось получить стоимость ремонта для товара"
        
        if result and result[1]:
            base_cost = result[1][0][0]
        else:
            base_cost = 5000  
        
        query = """
        INSERT INTO repairs (product_id, shop_id, start_date, actual_repair_cost, client_cost, serial_number, defect_description)
        VALUES (%s, %s, %s, %s, %s, %s, %s)
        RETURNING repair_id
        """
        actual_cost = 0 if is_warranty else base_cost
        client_cost = 0 if is_warranty else actual_cost
        params = (product_id, shop_id, datetime.now().date(), actual_cost, client_cost, serial_number, defect_description)
        
        success, result = self.execute_query(query, params, fetch=True)
        if success and result and result[1]:
            repair_id = result[1][0][0]
            return True, f"Ремонт #{repair_id} успешно добавлен!"
        return False, "Ошибка при добавлении ремонта"
    
    def complete_repair(self, repair_id, actual_cost):
        """Завершение ремонта"""
        query = """
        UPDATE repairs 
        SET end_date = %s, actual_repair_cost = %s 
        WHERE repair_id = %s AND end_date IS NULL
        RETURNING repair_id
        """
        params = (datetime.now().date(), actual_cost, repair_id)
        success, result = self.execute_query(query, params, fetch=True)
        if success and result and result[1]:
            return True, f"Ремонт #{repair_id} завершен!"
        return False, f"Не удалось завершить ремонт #{repair_id}"
    
    def get_products(self):
        query = """
        SELECT p.product_id, p.model_name, p.category, m.name as manufacturer 
        FROM products p 
        JOIN manufacturers m ON p.manufacturer_id = m.manufacturer_id 
        ORDER BY p.category, p.model_name
        """
        return self.execute_query(query, fetch=True)
    
    def get_shops(self):
        query = """
        SELECT s.shop_id, s.address, c.name as city,
               STRING_AGG(p.category, ', ') as specializations
        FROM repair_shops s 
        JOIN cities c ON s.city_id = c.city_id 
        LEFT JOIN shop_specializations sp ON s.shop_id = sp.shop_id
        LEFT JOIN products p ON sp.product_id = p.product_id
        GROUP BY s.shop_id, s.address, c.name
        ORDER BY c.name, s.address
        """
        return self.execute_query(query, fetch=True)
    
    def get_active_repairs(self):
        query = """
        SELECT 
            r.repair_id, 
            p.model_name, 
            m.name as manufacturer,
            c.name as city,
            s.address as shop_address,
            r.start_date,
            r.serial_number,
            r.defect_description,
            r.actual_repair_cost,
            CASE WHEN r.client_cost = 0 THEN 'Гарантия' ELSE 'Платный' END as repair_type
        FROM repairs r
        JOIN products p ON r.product_id = p.product_id
        JOIN manufacturers m ON p.manufacturer_id = m.manufacturer_id
        JOIN repair_shops s ON r.shop_id = s.shop_id
        JOIN cities c ON s.city_id = c.city_id
        WHERE r.end_date IS NULL
        ORDER BY r.start_date
        """
        return self.execute_query(query, fetch=True)

def print_table(title, columns, data, max_width=80):
    print(f"\n{'='*60}")
    print(f"📊 {title.upper()}")
    print('='*60)
    
    if not data:
        print("❌ Нет данных для отображения")
        return
    
    col_widths = []
    for i, col in enumerate(columns):
        max_len = len(str(col))
        for row in data:
            if i < len(row):
                max_len = max(max_len, len(str(row[i])))
        col_widths.append(min(max_len, 30))  
    
    header = " | ".join(str(col).ljust(col_widths[i]) for i, col in enumerate(columns))
    print(header)
    print('-' * len(header))
    
    for row in data:
        formatted_row = []
        for i, cell in enumerate(row):
            cell_str = str(cell)
            if len(cell_str) > col_widths[i]:
                cell_str = shorten(cell_str, width=col_widths[i], placeholder='...')
            formatted_row.append(cell_str.ljust(col_widths[i]))
        print(" | ".join(formatted_row))
    
    print(f"Всего записей: {len(data)}")

def print_unprofitability_rating(columns, data):
    print(f"\n{'🚨'*20}")
    print("🚨 РЕЙТИНГ УБЫТОЧНОСТИ ТОВАРОВ 🚨")
    print(f"{'🚨'*20}")
    
    if not data:
        print("📭 Нет данных о ремонтах")
        return
    
    formatted_data = []
    for row in data:
        formatted_row = list(row)
        
        formatted_row[4] = f"₽{float(row[4]):,.2f}"  
        formatted_row[5] = f"₽{float(row[5]):,.2f}"  
        formatted_row[6] = f"₽{float(row[6]):,.2f}"  
        formatted_data.append(formatted_row)
    
    pretty_columns = [
        'Модель 📱', 'Производитель 🏭', 'Категория 📦', 
        'Кол-во ремонтов 🔧', 'Общая стоимость 💰', 
        'Средняя стоимость 📈', 'Гарантийные расходы 🛡️', 'Рейтинг 📊'
    ]
    
    print_table("Рейтинг проблемных товаров", pretty_columns, formatted_data)

def print_city_coverage_report(columns, data):
    print(f"\n{'🏙️'*15}")
    print("🏙️  ОБЕСПЕЧЕННОСТЬ ГОРОДОВ МАСТЕРСКИМИ  🏙️")
    print(f"{'🏙️'*15}")
    
    if not data:
        print("📭 Нет данных о городах и мастерских")
        return
    
    formatted_data = []
    for row in data:
        formatted_row = list(row)

        coverage = row[5]
        if coverage >= 80:
            status = "✅"
        elif coverage >= 50:
            status = "⚠️"
        else:
            status = "❌"
        formatted_row[5] = f"{coverage}% {status}"
        formatted_data.append(formatted_row)
    
    pretty_columns = [
        'Город 🏘️', 'Категория 📦', 'Всего мастерских 🏢', 
        'Специализированные 🛠️', 'Выполнено ремонтов 🔧', 'Покрытие 📊'
    ]
    
    print_table("Обеспеченность городов", pretty_columns, formatted_data)

def print_products(columns, data):
    if not data:
        print("📭 Нет данных о товарах")
        return
    
    pretty_columns = ['ID 🔢', 'Модель 📱', 'Категория 📦', 'Производитель 🏭']
    print_table("Доступные товары", pretty_columns, data)

def print_shops(columns, data):
    if not data:
        print("📭 Нет данных о мастерских")
        return
    
    pretty_columns = ['ID 🔢', 'Адрес 🏢', 'Город 🏘️', 'Специализации 🛠️']
    print_table("Доступные мастерские", pretty_columns, data)

def print_active_repairs(columns, data):
    if not data:
        print("🎉 Нет активных ремонтов!")
        return
    
    pretty_columns = [
        'ID 🔢', 'Модель 📱', 'Производитель 🏭', 'Город 🏘️', 
        'Мастерская 🏢', 'Дата начала 📅', 'Серийный номер 🔐', 
        'Дефект ⚠️', 'Стоимость 💰', 'Тип 🏷️'
    ]
    print_table("Активные ремонты", pretty_columns, data)

def print_menu():
    print(f"\n{'⭐'*50}")
    print("⭐              СИСТЕМА ГАРАНТИЙНОГО РЕМОНТА              ⭐")
    print(f"{'⭐'*50}")
    print("1. 📊 Рейтинг убыточности товаров")
    print("2. 🏙️  Отчет по обеспеченности городов")
    print("3. ➕ Добавить новый ремонт")
    print("4. ✅ Завершить ремонт")
    print("5. 📋 Список активных ремонтов")
    print("6. 🏭 Список товаров")
    print("7. 🏢 Список мастерских")
    print("8. 🚪 Выход")
    print(f"{'─'*50}")
    return input("🎯 Выберите действие: ")

def main():
    db = RepairServiceDB()
    
    while True:
        try:
            choice = print_menu()
            
            if choice == '1':
                success, result = db.get_unprofitability_rating()
                if success and result:
                    columns, data = result
                    print_unprofitability_rating(columns, data)
                else:
                    print("❌ Ошибка при получении рейтинга убыточности")
            
            elif choice == '2':
                success, result = db.get_city_coverage_report()
                if success and result:
                    columns, data = result
                    print_city_coverage_report(columns, data)
                else:
                    print("❌ Ошибка при получении отчета по обеспеченности")
            
            elif choice == '3':
                print(f"\n{'➕'*10} ДОБАВЛЕНИЕ НОВОГО РЕМОНТА {'➕'*10}")
                
                success, result = db.get_products()
                if success and result:
                    columns, data = result
                    print_products(columns, data)
                    try:
                        product_id = int(input("\n🎯 Выберите ID товара: "))
                    except ValueError:
                        print("❌ Неверный формат ID!")
                        continue
                else:
                    print("❌ Не удалось загрузить список товаров")
                    continue
                
                success, result = db.get_shops()
                if success and result:
                    columns, data = result
                    print_shops(columns, data)
                    try:
                        shop_id = int(input("\n🎯 Выберите ID мастерской: "))
                    except ValueError:
                        print("❌ Неверный формат ID!")
                        continue
                else:
                    print("❌ Не удалось загрузить список мастерских")
                    continue
                
                serial_number = input("🔐 Серийный номер: ")
                defect_description = input("⚠️  Описание дефекта: ")
                is_warranty = input("🛡️  Гарантийный ремонт? (y/n): ").lower() == 'y'
                
                success, message = db.add_repair(product_id, shop_id, serial_number, defect_description, is_warranty)
                if success:
                    print(f"✅ {message}")
                else:
                    print(f"❌ {message}")
            
            elif choice == '4':
                success, result = db.get_active_repairs()
                if success and result:
                    columns, data = result
                    print_active_repairs(columns, data)
                    try:
                        repair_id = int(input("\n🎯 Выберите ID ремонта для завершения: "))
                        actual_cost = float(input("💰 Фактическая стоимость ремонта: "))
                        
                        success, message = db.complete_repair(repair_id, actual_cost)
                        if success:
                            print(f"✅ {message}")
                        else:
                            print(f"❌ {message}")
                    except ValueError:
                        print("❌ Неверный формат данных!")
                else:
                    print("🎉 Нет активных ремонтов для завершения!")
            
            elif choice == '5':
                success, result = db.get_active_repairs()
                if success and result:
                    columns, data = result
                    print_active_repairs(columns, data)
                else:
                    print("❌ Ошибка при получении списка ремонтов")
            
            elif choice == '6':
                success, result = db.get_products()
                if success and result:
                    columns, data = result
                    print_products(columns, data)
                else:
                    print("❌ Ошибка при получении списка товаров")
            
            elif choice == '7':
                success, result = db.get_shops()
                if success and result:
                    columns, data = result
                    print_shops(columns, data)
                else:
                    print("❌ Ошибка при получении списка мастерских")
            
            elif choice == '8':
                print(f"\n{'👋'*10} До свидания! {'👋'*10}")
                break
            
            else:
                print("❌ Неверный выбор! Попробуйте снова.")
        
        except KeyboardInterrupt:
            print(f"\n\n{'👋'*10} Программа прервана пользователем {'👋'*10}")
            break
        except Exception as e:
            print(f"❌ Неожиданная ошибка: {e}")

if __name__ == "__main__":
    main()