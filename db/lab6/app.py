
import sys
import psycopg2
from datetime import datetime
from kivy.app import App
from kivy.uix.screenmanager import ScreenManager, Screen
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.gridlayout import GridLayout
from kivy.uix.scrollview import ScrollView
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.textinput import TextInput
from kivy.uix.spinner import Spinner
from kivy.uix.checkbox import CheckBox
from kivy.uix.popup import Popup
from kivy.core.window import Window
from kivy.clock import Clock
from kivy.metrics import dp
from kivy.properties import ListProperty, NumericProperty

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
        SELECT 
            c.name as city,
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
        GROUP BY c.name, p.category
        ORDER BY c.name, coverage_percentage DESC;
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
        SELECT s.shop_id, s.address, c.name as city
        FROM repair_shops s 
        JOIN cities c ON s.city_id = c.city_id 
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

class CustomLabel(Label):
    pass

class CustomButton(Button):
    pass

class DataTable(GridLayout):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.cols = 1
        self.size_hint_y = None
        self.bind(minimum_height=self.setter('height'))
    
    def populate(self, columns, data):
        self.clear_widgets()
        
        header_layout = GridLayout(cols=len(columns), size_hint_y=None, height=dp(40))
        for col in columns:
            header = CustomLabel(
                text=str(col), 
                bold=True,
                size_hint_y=None,
                height=dp(40)
            )
            header_layout.add_widget(header)
        self.add_widget(header_layout)
        
        for row in data:
            row_layout = GridLayout(cols=len(columns), size_hint_y=None, height=dp(35))
            for cell in row:
                cell_label = CustomLabel(
                    text=str(cell),
                    size_hint_y=None,
                    height=dp(35),
                    text_size=(None, None),
                    halign='left',
                    valign='middle'
                )
                cell_label.bind(texture_size=cell_label.setter('size'))
                row_layout.add_widget(cell_label)
            self.add_widget(row_layout)

class BaseScreen(Screen):
    def __init__(self, db, **kwargs):
        super().__init__(**kwargs)
        self.db = db
    
    def show_message(self, title, message):
        content = BoxLayout(orientation='vertical', padding=dp(10), spacing=dp(10))
        content.add_widget(Label(text=message))
        btn = CustomButton(text='OK', size_hint_y=None, height=dp(40))
        popup = Popup(title=title, content=content, size_hint=(0.6, 0.3))
        btn.bind(on_press=popup.dismiss)
        content.add_widget(btn)
        popup.open()

class MainMenuScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'main_menu'
        
        layout = BoxLayout(orientation='vertical', padding=dp(20), spacing=dp(10))

        title = CustomLabel(
            text='🛠️ Система гарантийного ремонта',
            font_size='24sp',
            size_hint_y=None,
            height=dp(60)
        )
        layout.add_widget(title)

        menu_buttons = [
            ('📈 Рейтинг убыточности', 'unprofitability'),
            ('🏙️ Обеспеченность городов', 'coverage'),
            ('🔧 Активные ремонты', 'active_repairs'),
            ('➕ Новый ремонт', 'add_repair'),
            ('✅ Завершить ремонт', 'complete_repair'),
            ('🏭 Список товаров', 'products'),
            ('🏢 Список мастерских', 'shops')
        ]
        
        for text, screen_name in menu_buttons:
            btn = CustomButton(
                text=text,
                size_hint_y=None,
                height=dp(50)
            )
            btn.bind(on_press=lambda instance, sn=screen_name: self.switch_screen(sn))
            layout.add_widget(btn)
        
        exit_btn = CustomButton(
            text='🚪 Выход',
            size_hint_y=None,
            height=dp(50),
            background_color=(0.8, 0.2, 0.2, 1)
        )
        exit_btn.bind(on_press=self.exit_app)
        layout.add_widget(exit_btn)
        
        self.add_widget(layout)
    
    def switch_screen(self, screen_name):
        self.manager.current = screen_name
    
    def exit_app(self, instance):
        App.get_running_app().stop()

class UnprofitabilityScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'unprofitability'
        self.init_ui()
        Clock.schedule_once(lambda dt: self.load_data())
    
    def init_ui(self):
        layout = BoxLayout(orientation='vertical', padding=dp(10), spacing=dp(10))

        header = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        back_btn = CustomButton(text='⬅️ Назад', size_hint_x=None, width=dp(100))
        back_btn.bind(on_press=self.go_back)
        header.add_widget(back_btn)
        
        title = CustomLabel(text='📈 Рейтинг убыточности товаров', font_size='20sp')
        header.add_widget(title)
        header.add_widget(Label()) 
        
        layout.add_widget(header)
        
        refresh_btn = CustomButton(text='🔄 Обновить', size_hint_y=None, height=dp(40))
        refresh_btn.bind(on_press=self.load_data)
        layout.add_widget(refresh_btn)
        
        scroll = ScrollView()
        self.table = DataTable()
        scroll.add_widget(self.table)
        layout.add_widget(scroll)
        
        self.add_widget(layout)
    
    def go_back(self, instance):
        self.manager.current = 'main_menu'
    
    def load_data(self, instance=None):
        success, result = self.db.get_unprofitability_rating()
        if success and result:
            columns, data = result
            formatted_data = []
            for row in data:
                formatted_row = list(row)
                formatted_row[4] = f"₽{float(row[4]):,.2f}"
                formatted_row[5] = f"₽{float(row[5]):,.2f}"
                formatted_row[6] = f"₽{float(row[6]):,.2f}"
                formatted_data.append(formatted_row)
            
            pretty_columns = ['Модель', 'Производитель', 'Категория', 'Ремонтов', 
                            'Общая стоимость', 'Средняя стоимость', 'Гарантийные расходы', 'Рейтинг']
            self.table.populate(pretty_columns, formatted_data)
        else:
            self.show_message('Ошибка', 'Не удалось загрузить данные')

class CoverageScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'coverage'
        self.init_ui()
        Clock.schedule_once(lambda dt: self.load_data())
    
    def init_ui(self):
        layout = BoxLayout(orientation='vertical', padding=dp(10), spacing=dp(10))
        
        header = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        back_btn = CustomButton(text='⬅️ Назад', size_hint_x=None, width=dp(100))
        back_btn.bind(on_press=self.go_back)
        header.add_widget(back_btn)
        
        title = CustomLabel(text='🏙️ Обеспеченность городов', font_size='20sp')
        header.add_widget(title)
        header.add_widget(Label())
        
        layout.add_widget(header)
        
        refresh_btn = CustomButton(text='🔄 Обновить', size_hint_y=None, height=dp(40))
        refresh_btn.bind(on_press=self.load_data)
        layout.add_widget(refresh_btn)
        
        scroll = ScrollView()
        self.table = DataTable()
        scroll.add_widget(self.table)
        layout.add_widget(scroll)
        
        self.add_widget(layout)
    
    def go_back(self, instance):
        self.manager.current = 'main_menu'
    
    def load_data(self, instance=None):
        success, result = self.db.get_city_coverage_report()
        if success and result:
            columns, data = result
            formatted_data = []
            for row in data:
                formatted_row = list(row)
                coverage = row[5]
                status = "✅" if coverage >= 80 else "⚠️" if coverage >= 50 else "❌"
                formatted_row[5] = f"{coverage}% {status}"
                formatted_data.append(formatted_row)
            
            pretty_columns = ['Город', 'Категория', 'Мастерских', 'Специализаций', 'Ремонтов', 'Покрытие']
            self.table.populate(pretty_columns, formatted_data)
        else:
            self.show_message('Ошибка', 'Не удалось загрузить данные')

class ActiveRepairsScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'active_repairs'
        self.init_ui()
        Clock.schedule_once(lambda dt: self.load_data())
    
    def init_ui(self):
        layout = BoxLayout(orientation='vertical', padding=dp(10), spacing=dp(10))
        
        header = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        back_btn = CustomButton(text='⬅️ Назад', size_hint_x=None, width=dp(100))
        back_btn.bind(on_press=self.go_back)
        header.add_widget(back_btn)
        
        title = CustomLabel(text='🔧 Активные ремонты', font_size='20sp')
        header.add_widget(title)
        header.add_widget(Label())
        
        layout.add_widget(header)
        
        refresh_btn = CustomButton(text='🔄 Обновить', size_hint_y=None, height=dp(40))
        refresh_btn.bind(on_press=self.load_data)
        layout.add_widget(refresh_btn)
        
        scroll = ScrollView()
        self.table = DataTable()
        scroll.add_widget(self.table)
        layout.add_widget(scroll)
        
        self.add_widget(layout)
    
    def go_back(self, instance):
        self.manager.current = 'main_menu'
    
    def load_data(self, instance=None):
        success, result = self.db.get_active_repairs()
        if success and result:
            columns, data = result
            pretty_columns = ['ID', 'Модель', 'Производитель', 'Город', 'Мастерская', 
                            'Дата начала', 'Серийный номер', 'Дефект', 'Стоимость', 'Тип']
            self.table.populate(pretty_columns, data)
        else:
            self.show_message('Ошибка', 'Не удалось загрузить данные')

class AddRepairScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'add_repair'
        self.products_data = {}
        self.shops_data = {}
        self.init_ui()
        Clock.schedule_once(lambda dt: self.load_products_and_shops())
    
    def init_ui(self):
        layout = BoxLayout(orientation='vertical', padding=dp(20), spacing=dp(10))

        header = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        back_btn = CustomButton(text='⬅️ Назад', size_hint_x=None, width=dp(100))
        back_btn.bind(on_press=self.go_back)
        header.add_widget(back_btn)
        
        title = CustomLabel(text='➕ Добавить ремонт', font_size='20sp')
        header.add_widget(title)
        header.add_widget(Label())
        
        layout.add_widget(header)

        form_layout = GridLayout(cols=2, spacing=dp(10), size_hint_y=None)
        form_layout.height = dp(250)

        form_layout.add_widget(CustomLabel(text='Товар:'))
        self.product_spinner = Spinner(
            text='Выберите товар', 
            size_hint_y=None, 
            height=dp(40),
            background_color=(0.9, 0.9, 1, 1)
        )
        form_layout.add_widget(self.product_spinner)

        form_layout.add_widget(CustomLabel(text='Мастерская:'))
        self.shop_spinner = Spinner(
            text='Выберите мастерскую', 
            size_hint_y=None, 
            height=dp(40),
            background_color=(0.9, 0.9, 1, 1)
        )
        form_layout.add_widget(self.shop_spinner)

        form_layout.add_widget(CustomLabel(text='Серийный номер:'))
        self.serial_input = TextInput(
            multiline=False, 
            size_hint_y=None, 
            height=dp(40),
            hint_text='Введите серийный номер'
        )
        form_layout.add_widget(self.serial_input)

        form_layout.add_widget(CustomLabel(text='Описание дефекта:'))
        self.defect_input = TextInput(
            multiline=True, 
            size_hint_y=None, 
            height=dp(80),
            hint_text='Опишите проблему'
        )
        form_layout.add_widget(self.defect_input)
        
        form_layout.add_widget(CustomLabel(text='Гарантийный ремонт:'))
        warranty_layout = BoxLayout(orientation='horizontal')
        self.warranty_check = CheckBox(size_hint_x=None, width=dp(40))
        warranty_layout.add_widget(self.warranty_check)
        warranty_layout.add_widget(CustomLabel(text='Да'))
        form_layout.add_widget(warranty_layout)
        
        layout.add_widget(form_layout)
        
        btn_layout = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        add_btn = CustomButton(text='💾 Добавить ремонт')
        add_btn.bind(on_press=self.add_repair)
        clear_btn = CustomButton(text='🗑️ Очистить')
        clear_btn.bind(on_press=self.clear_form)
        
        btn_layout.add_widget(add_btn)
        btn_layout.add_widget(clear_btn)
        layout.add_widget(btn_layout)
        
        self.add_widget(layout)
    
    def go_back(self, instance):
        self.manager.current = 'main_menu'
    
    def load_products_and_shops(self):
        success, result = self.db.get_products()
        if success and result:
            columns, data = result
            product_values = []
            self.products_data = {}
            for row in data:
                display_text = f"{row[1]} ({row[2]}) - {row[3]}"
                product_values.append(display_text)
                self.products_data[display_text] = row[0]
            self.product_spinner.values = product_values
        
        success, result = self.db.get_shops()
        if success and result:
            columns, data = result
            shop_values = []
            self.shops_data = {}
            for row in data:
                display_text = f"{row[1]} ({row[2]})"
                shop_values.append(display_text)
                self.shops_data[display_text] = row[0]
            self.shop_spinner.values = shop_values
    
    def add_repair(self, instance):
        if (self.product_spinner.text == 'Выберите товар' or 
            self.shop_spinner.text == 'Выберите мастерскую' or
            not self.serial_input.text.strip()):
            self.show_message('Ошибка', 'Заполните все обязательные поля')
            return
        
        product_id = self.products_data.get(self.product_spinner.text)
        shop_id = self.shops_data.get(self.shop_spinner.text)
        serial_number = self.serial_input.text.strip()
        defect_description = self.defect_input.text.strip()
        is_warranty = self.warranty_check.active
        
        success, message = self.db.add_repair(product_id, shop_id, serial_number, defect_description, is_warranty)
        
        if success:
            self.show_message('Успех', message)
            self.clear_form()
        else:
            self.show_message('Ошибка', message)
    
    def clear_form(self, instance=None):
        self.product_spinner.text = 'Выберите товар'
        self.shop_spinner.text = 'Выберите мастерскую'
        self.serial_input.text = ''
        self.defect_input.text = ''
        self.warranty_check.active = False

class CompleteRepairScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'complete_repair'
        self.selected_repair_id = None
        self.full_data = []
        self.init_ui()
        Clock.schedule_once(lambda dt: self.load_data())
    
    def init_ui(self):
        layout = BoxLayout(orientation='vertical', padding=dp(10), spacing=dp(10))
        
        header = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        back_btn = CustomButton(text='⬅️ Назад', size_hint_x=None, width=dp(100))
        back_btn.bind(on_press=self.go_back)
        header.add_widget(back_btn)
        
        title = CustomLabel(text='✅ Завершить ремонт', font_size='20sp')
        header.add_widget(title)
        header.add_widget(Label())
        
        layout.add_widget(header)
        
        refresh_btn = CustomButton(text='🔄 Обновить список', size_hint_y=None, height=dp(40))
        refresh_btn.bind(on_press=self.load_data)
        layout.add_widget(refresh_btn)
        
        selected_layout = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(40))
        selected_layout.add_widget(CustomLabel(text='Выбранный ремонт:'))
        self.selected_label = CustomLabel(text='Не выбран', color=(0.2, 0.6, 1, 1))
        selected_layout.add_widget(self.selected_label)
        layout.add_widget(selected_layout)
        
        cost_layout = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(40))
        cost_layout.add_widget(CustomLabel(text='Фактическая стоимость:'))
        self.cost_input = TextInput(
            multiline=False, 
            size_hint_y=None, 
            height=dp(40),
            hint_text='0.00',
            input_filter='float'
        )
        cost_layout.add_widget(self.cost_input)
        layout.add_widget(cost_layout)
        
        complete_btn = CustomButton(
            text='✅ Завершить ремонт', 
            size_hint_y=None, 
            height=dp(50),
            background_color=(0.2, 0.8, 0.2, 1)
        )
        complete_btn.bind(on_press=self.complete_repair)
        layout.add_widget(complete_btn)
        
        scroll = ScrollView()
        self.table = DataTable()
        scroll.add_widget(self.table)
        layout.add_widget(scroll)
        
        self.add_widget(layout)
    
    def go_back(self, instance):
        self.manager.current = 'main_menu'
    
    def load_data(self, instance=None):
        success, result = self.db.get_active_repairs()
        if success and result:
            columns, data = result
            self.full_data = data
            short_data = [row[:6] for row in data]  
            pretty_columns = ['ID', 'Модель', 'Производитель', 'Город', 'Мастерская', 'Дата начала']
            self.table.populate(pretty_columns, short_data)
        else:
            self.show_message('Ошибка', 'Не удалось загрузить данные')
    
    def on_table_touch(self, instance, touch):
        if instance.collide_point(*touch.pos) and touch.button == 'left':
            pass
    
    def complete_repair(self, instance):
        if not self.selected_repair_id:
            self.show_message('Ошибка', 'Выберите ремонт для завершения')
            return
        
        try:
            actual_cost = float(self.cost_input.text) if self.cost_input.text.strip() else 0.0
        except ValueError:
            self.show_message('Ошибка', 'Введите корректную стоимость')
            return
        
        success, message = self.db.complete_repair(self.selected_repair_id, actual_cost)
        
        if success:
            self.show_message('Успех', message)
            self.load_data()
            self.selected_repair_id = None
            self.selected_label.text = 'Не выбран'
            self.cost_input.text = ''
        else:
            self.show_message('Ошибка', message)

class ProductsScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'products'
        self.init_ui()
        Clock.schedule_once(lambda dt: self.load_data())
    
    def init_ui(self):
        layout = BoxLayout(orientation='vertical', padding=dp(10), spacing=dp(10))
        
        header = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        back_btn = CustomButton(text='⬅️ Назад', size_hint_x=None, width=dp(100))
        back_btn.bind(on_press=self.go_back)
        header.add_widget(back_btn)
        
        title = CustomLabel(text='🏭 Список товаров', font_size='20sp')
        header.add_widget(title)
        header.add_widget(Label())
        
        layout.add_widget(header)
        
        refresh_btn = CustomButton(text='🔄 Обновить', size_hint_y=None, height=dp(40))
        refresh_btn.bind(on_press=self.load_data)
        layout.add_widget(refresh_btn)
        
        scroll = ScrollView()
        self.table = DataTable()
        scroll.add_widget(self.table)
        layout.add_widget(scroll)
        
        self.add_widget(layout)
    
    def go_back(self, instance):
        self.manager.current = 'main_menu'
    
    def load_data(self, instance=None):
        success, result = self.db.get_products()
        if success and result:
            columns, data = result
            pretty_columns = ['ID', 'Модель', 'Категория', 'Производитель']
            self.table.populate(pretty_columns, data)
        else:
            self.show_message('Ошибка', 'Не удалось загрузить данные')

class ShopsScreen(BaseScreen):
    def __init__(self, db, **kwargs):
        super().__init__(db, **kwargs)
        self.name = 'shops'
        self.init_ui()
        Clock.schedule_once(lambda dt: self.load_data())
    
    def init_ui(self):
        layout = BoxLayout(orientation='vertical', padding=dp(10), spacing=dp(10))
        
        header = BoxLayout(orientation='horizontal', size_hint_y=None, height=dp(50))
        back_btn = CustomButton(text='⬅️ Назад', size_hint_x=None, width=dp(100))
        back_btn.bind(on_press=self.go_back)
        header.add_widget(back_btn)
        
        title = CustomLabel(text='🏢 Список мастерских', font_size='20sp')
        header.add_widget(title)
        header.add_widget(Label())
        
        layout.add_widget(header)
        
        refresh_btn = CustomButton(text='🔄 Обновить', size_hint_y=None, height=dp(40))
        refresh_btn.bind(on_press=self.load_data)
        layout.add_widget(refresh_btn)
        
        scroll = ScrollView()
        self.table = DataTable()
        scroll.add_widget(self.table)
        layout.add_widget(scroll)
        
        self.add_widget(layout)
    
    def go_back(self, instance):
        self.manager.current = 'main_menu'
    
    def load_data(self, instance=None):
        success, result = self.db.get_shops()
        if success and result:
            columns, data = result
            pretty_columns = ['ID', 'Адрес', 'Город']
            self.table.populate(pretty_columns, data)
        else:
            self.show_message('Ошибка', 'Не удалось загрузить данные')

class RepairServiceApp(App):
    def build(self):
        self.title = "🛠️ Система гарантийного ремонта"

        self.db = RepairServiceDB()

        sm = ScreenManager()
        
        screens = [
            MainMenuScreen(self.db),
            UnprofitabilityScreen(self.db),
            CoverageScreen(self.db),
            ActiveRepairsScreen(self.db),
            AddRepairScreen(self.db),
            CompleteRepairScreen(self.db),
            ProductsScreen(self.db),
            ShopsScreen(self.db)
        ]
        
        for screen in screens:
            sm.add_widget(screen)
        
        return sm

if __name__ == '__main__':
    RepairServiceApp().run()