from fastapi import FastAPI, Depends, HTTPException, Request, Form
from fastapi.templating import Jinja2Templates
from fastapi.responses import HTMLResponse, RedirectResponse
from sqlalchemy.orm import Session
from sqlalchemy import text  
from typing import List
import uvicorn
import logging
import os
from pathlib import Path
from datetime import date

import models
import schemas
import crud
import services
from database import SessionLocal, engine, get_db

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def init_db():
    db = SessionLocal()
    try:
        if db.query(models.City).count() > 0:
            logger.info("✅ База данных уже содержит данные")
            return
        
        logger.info("🔄 Инициализация базы данных...")
        
        cities_data = [
            models.City(name='Москва'),
            models.City(name='Санкт-Петербург'),
            models.City(name='Новосибирск'),
            models.City(name='Екатеринбург'),
            models.City(name='Казань'),
            models.City(name='Нижний Новгород'),
            models.City(name='Ростов-на-Дону'),
            models.City(name='Краснодар')
        ]
        db.add_all(cities_data)
        db.flush()  
        
        manufacturers_data = [
            models.Manufacturer(name='Xiaomi', contact_info='Китай, Пекин, ул. Центральная, 1. Тел.: +86-10-1234567'),
            models.Manufacturer(name='Samsung', contact_info='Южная Корея, Сувон. Тел.: +82-31-200-1111'),
            models.Manufacturer(name='Bosch', contact_info='Германия, Штутгарт. Тел.: +49-711-811-0'),
            models.Manufacturer(name='LG', contact_info='Южная Корея, Сеул. Тел.: +82-2-3777-1111'),
            models.Manufacturer(name='Apple', contact_info='США, Купертино. Тел.: +1-408-996-1010'),
            models.Manufacturer(name='Huawei', contact_info='Китай, Шэньчжэнь. Тел.: +86-755-28780808'),
            models.Manufacturer(name='Sony', contact_info='Япония, Токио. Тел.: +81-3-6748-2111'),
            models.Manufacturer(name='Lenovo', contact_info='Китай, Пекин. Тел.: +86-10-58851111')
        ]
        db.add_all(manufacturers_data)
        db.flush()
        
        products_data = [
            models.Product(manufacturer_id=1, model_name='Redmi Note 13', category='смартфоны', warranty_period=365, base_repair_cost=5000.00),
            models.Product(manufacturer_id=2, model_name='Galaxy S24', category='смартфоны', warranty_period=730, base_repair_cost=15000.00),
            models.Product(manufacturer_id=3, model_name='KGN39VI32R', category='холодильники', warranty_period=1095, base_repair_cost=12000.00),
            models.Product(manufacturer_id=1, model_name='Robot Vacuum X10', category='пылесосы', warranty_period=180, base_repair_cost=8000.00),
            models.Product(manufacturer_id=4, model_name='OLED55C3', category='телевизоры', warranty_period=730, base_repair_cost=20000.00),
            models.Product(manufacturer_id=5, model_name='iPhone 15 Pro', category='смартфоны', warranty_period=365, base_repair_cost=25000.00),
            models.Product(manufacturer_id=3, model_name='WTG86401RU', category='стиральные машины', warranty_period=1095, base_repair_cost=15000.00),
            models.Product(manufacturer_id=6, model_name='MatePad 11', category='планшеты', warranty_period=365, base_repair_cost=9000.00),
            models.Product(manufacturer_id=7, model_name='WH-1000XM5', category='наушники', warranty_period=365, base_repair_cost=7000.00),
            models.Product(manufacturer_id=8, model_name='ThinkPad X1', category='ноутбуки', warranty_period=730, base_repair_cost=18000.00),
            models.Product(manufacturer_id=2, model_name='Galaxy Tab S9', category='планшеты', warranty_period=730, base_repair_cost=12000.00),
            models.Product(manufacturer_id=5, model_name='MacBook Air M2', category='ноутбуки', warranty_period=365, base_repair_cost=22000.00)
        ]
        db.add_all(products_data)
        db.flush()
        
        repair_shops_data = [
            models.RepairShop(city_id=1, address='ул. Тверская, 25', contact_phone='+7-495-111-22-33'),
            models.RepairShop(city_id=1, address='пр-т Мира, 100', contact_phone='+7-495-444-55-66'),
            models.RepairShop(city_id=2, address='Невский пр-т, 50', contact_phone='+7-812-777-88-99'),
            models.RepairShop(city_id=3, address='ул. Ленина, 1', contact_phone='+7-383-123-45-67'),
            models.RepairShop(city_id=4, address='ул. Малышева, 45', contact_phone='+7-343-555-66-77'),
            models.RepairShop(city_id=5, address='ул. Баумана, 15', contact_phone='+7-843-999-00-11'),
            models.RepairShop(city_id=6, address='ул. Большая Покровская, 35', contact_phone='+7-831-222-33-44'),
            models.RepairShop(city_id=7, address='ул. Большая Садовая, 15', contact_phone='+7-863-444-55-66'),
            models.RepairShop(city_id=8, address='ул. Красная, 100', contact_phone='+7-861-777-88-99')
        ]
        db.add_all(repair_shops_data)
        db.flush()
        
        specializations_data = [
            models.ShopSpecialization(shop_id=1, product_id=1),
            models.ShopSpecialization(shop_id=1, product_id=2),
            models.ShopSpecialization(shop_id=1, product_id=6),
            models.ShopSpecialization(shop_id=2, product_id=3),
            models.ShopSpecialization(shop_id=2, product_id=7),
            models.ShopSpecialization(shop_id=2, product_id=10),
            models.ShopSpecialization(shop_id=3, product_id=1),
            models.ShopSpecialization(shop_id=3, product_id=4),
            models.ShopSpecialization(shop_id=3, product_id=8),
            models.ShopSpecialization(shop_id=4, product_id=2),
            models.ShopSpecialization(shop_id=4, product_id=3),
            models.ShopSpecialization(shop_id=4, product_id=5),
            models.ShopSpecialization(shop_id=5, product_id=5),
            models.ShopSpecialization(shop_id=5, product_id=6),
            models.ShopSpecialization(shop_id=5, product_id=9),
            models.ShopSpecialization(shop_id=6, product_id=1),
            models.ShopSpecialization(shop_id=6, product_id=2),
            models.ShopSpecialization(shop_id=6, product_id=6),
            models.ShopSpecialization(shop_id=6, product_id=11),
            models.ShopSpecialization(shop_id=7, product_id=3),
            models.ShopSpecialization(shop_id=7, product_id=7),
            models.ShopSpecialization(shop_id=7, product_id=10),
            models.ShopSpecialization(shop_id=8, product_id=4),
            models.ShopSpecialization(shop_id=8, product_id=8),
            models.ShopSpecialization(shop_id=8, product_id=9),
            models.ShopSpecialization(shop_id=9, product_id=5),
            models.ShopSpecialization(shop_id=9, product_id=6),
            models.ShopSpecialization(shop_id=9, product_id=12)
        ]
        db.add_all(specializations_data)
        db.flush()
        
        repairs_data = [
            models.Repair(product_id=1, shop_id=1, start_date=date(2024,1,15), end_date=date(2024,1,20), actual_repair_cost=3000.00, client_cost=0.00, serial_number='XN13123456', defect_description='Разбит экран, гарантийный случай'),
            models.Repair(product_id=2, shop_id=1, start_date=date(2024,2,1), end_date=date(2024,2,5), actual_repair_cost=0.00, client_cost=12000.00, serial_number='GS24123456', defect_description='Замена аккумулятора, не гарантия'),
            models.Repair(product_id=3, shop_id=2, start_date=date(2024,2,10), end_date=date(2024,2,15), actual_repair_cost=8500.00, client_cost=0.00, serial_number='BOSCH98765', defect_description='Утечка фреона, гарантийный ремонт'),
            models.Repair(product_id=4, shop_id=3, start_date=date(2024,1,20), end_date=date(2024,1,25), actual_repair_cost=5000.00, client_cost=5000.00, serial_number='X10054321', defect_description='Не работает двигатель, постгарантия'),
            models.Repair(product_id=6, shop_id=5, start_date=date(2024,3,1), end_date=None, actual_repair_cost=15000.00, client_cost=0.00, serial_number='IP15P88888', defect_description='Не работает камера, гарантийный'),
            models.Repair(product_id=2, shop_id=6, start_date=date(2024,2,20), end_date=date(2024,2,25), actual_repair_cost=2000.00, client_cost=2000.00, serial_number='GS24987654', defect_description='Замена разъема зарядки'),
            models.Repair(product_id=8, shop_id=3, start_date=date(2024,3,5), end_date=date(2024,3,10), actual_repair_cost=6000.00, client_cost=6000.00, serial_number='HUA888123', defect_description='Разбит экран, не гарантия'),
            models.Repair(product_id=10, shop_id=7, start_date=date(2024,2,15), end_date=None, actual_repair_cost=12000.00, client_cost=12000.00, serial_number='LEN456789', defect_description='Не работает клавиатура'),
            models.Repair(product_id=5, shop_id=5, start_date=date(2024,3,10), end_date=date(2024,3,12), actual_repair_cost=0.00, client_cost=15000.00, serial_number='LG55C3333', defect_description='Поломка матрицы'),
            models.Repair(product_id=7, shop_id=2, start_date=date(2024,2,28), end_date=None, actual_repair_cost=8000.00, client_cost=0.00, serial_number='BOS111222', defect_description='Не отжимает, гарантийный'),
            models.Repair(product_id=9, shop_id=8, start_date=date(2024,3,8), end_date=date(2024,3,12), actual_repair_cost=4000.00, client_cost=4000.00, serial_number='SONY555666', defect_description='Не работает шумоподавление'),
            models.Repair(product_id=11, shop_id=6, start_date=date(2024,3,3), end_date=None, actual_repair_cost=9000.00, client_cost=0.00, serial_number='SAMS999888', defect_description='Не включается, гарантийный'),
            models.Repair(product_id=12, shop_id=9, start_date=date(2024,2,25), end_date=date(2024,3,5), actual_repair_cost=18000.00, client_cost=18000.00, serial_number='MAC123456', defect_description='Замена SSD')
        ]
        db.add_all(repairs_data)
        
        db.commit()
        logger.info("✅ Начальные данные успешно добавлены в базу данных")
        
    except Exception as e:
        db.rollback()
        logger.error(f"❌ Ошибка при инициализации базы данных: {e}")
        raise
    finally:
        db.close()

try:
    models.Base.metadata.create_all(bind=engine)
    logger.info("✅ Таблицы базы данных созданы/проверены")
    
    init_db()
    
except Exception as e:
    logger.error(f"❌ Ошибка при создании таблиц: {e}")

app = FastAPI(title="Repair Service API", version="1.0.0")

base_dir = Path(__file__).parent
templates = Jinja2Templates(directory=os.path.join(base_dir, "templates"))

@app.get("/api/unprofitability-rating", response_model=List[schemas.UnprofitabilityRating])
def get_unprofitability_rating(limit: int = 15, db: Session = Depends(get_db)):
    return services.get_unprofitability_rating(db, limit)

@app.get("/api/city-coverage", response_model=List[schemas.CityCoverage])
def get_city_coverage(db: Session = Depends(get_db)):
    return services.get_city_coverage_report(db)

@app.get("/api/categories/stats", response_model=List[schemas.CategoryStats])
def get_category_statistics(db: Session = Depends(get_db)):
    return services.get_category_statistics(db)

@app.get("/api/products", response_model=List[schemas.Product])
def get_products(skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
    return crud.get_products(db, skip=skip, limit=limit)

@app.get("/api/shops", response_model=List[schemas.RepairShop])
def get_shops(skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
    return crud.get_repair_shops(db, skip=skip, limit=limit)

@app.get("/api/repairs", response_model=List[schemas.Repair])
def get_repairs(active_only: bool = False, skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
    return crud.get_repairs(db, skip=skip, limit=limit, active_only=active_only)

@app.post("/api/repairs", response_model=schemas.Repair)
def create_repair(repair: schemas.RepairCreate, db: Session = Depends(get_db)):
    db_repair = crud.create_repair(db, repair)
    if db_repair is None:
        raise HTTPException(status_code=400, detail="Не удалось создать ремонт")
    return db_repair

@app.post("/api/repairs/{repair_id}/complete", response_model=schemas.Repair)
def complete_repair(repair_id: int, complete_data: schemas.RepairComplete, db: Session = Depends(get_db)):
    db_repair = crud.complete_repair(db, repair_id, complete_data.actual_cost)
    if db_repair is None:
        raise HTTPException(status_code=404, detail="Ремонт не найден или уже завершен")
    return db_repair

@app.get("/", response_class=HTMLResponse)
def read_root(request: Request, db: Session = Depends(get_db)):
    active_repairs = db.query(models.Repair).filter(models.Repair.end_date.is_(None)).count()
    completed_repairs = db.query(models.Repair).filter(models.Repair.end_date.isnot(None)).count()
    products_count = db.query(models.Product).count()
    shops_count = db.query(models.RepairShop).count()
    
    recent_repairs = db.query(models.Repair).order_by(models.Repair.start_date.desc()).limit(5).all()
    
    stats = {
        "active_repairs": active_repairs,
        "completed_repairs": completed_repairs,
        "products_count": products_count,
        "shops_count": shops_count
    }
    
    return templates.TemplateResponse("index.html", {
        "request": request,
        "stats": stats,
        "recent_repairs": recent_repairs
    })

@app.get("/unprofitability", response_class=HTMLResponse)
def unprofitability_page(request: Request, db: Session = Depends(get_db)):
    rating = services.get_unprofitability_rating(db)
    return templates.TemplateResponse("unprofitability.html", {
        "request": request,
        "rating": rating
    })

@app.get("/coverage", response_class=HTMLResponse)
def coverage_page(request: Request, db: Session = Depends(get_db)):
    coverage = services.get_city_coverage_report(db)
    return templates.TemplateResponse("coverage.html", {
        "request": request,
        "coverage": coverage
    })

@app.get("/repairs", response_class=HTMLResponse)
def repairs_page(request: Request, db: Session = Depends(get_db)):
    repairs = crud.get_repairs(db, active_only=True)
    all_repairs = crud.get_repairs(db, active_only=False, limit=50)
    products = crud.get_products(db)
    shops = crud.get_repair_shops(db)
    
    stats = crud.get_repairs_stats(db)
    
    return templates.TemplateResponse("repairs.html", {
        "request": request,
        "repairs": repairs,
        "all_repairs": all_repairs,
        "products": products,
        "shops": shops,
        "stats": stats
    })

@app.get("/products", response_class=HTMLResponse)
def products_page(request: Request, db: Session = Depends(get_db)):
    products = crud.get_products(db)
    # Получаем уникальные категории
    categories = list(set([product.category for product in products]))
    return templates.TemplateResponse("products.html", {
        "request": request,
        "products": products,
        "categories": sorted(categories)
    })

@app.get("/shops", response_class=HTMLResponse)
def shops_page(request: Request, db: Session = Depends(get_db)):
    shops = crud.get_repair_shops(db)
    cities = list(set([shop.city.name for shop in shops]))
    return templates.TemplateResponse("shops.html", {
        "request": request,
        "shops": shops,
        "cities": sorted(cities)
    })

@app.post("/repairs/add")
def add_repair(
    request: Request,
    product_id: int = Form(...),
    shop_id: int = Form(...),
    serial_number: str = Form(""),
    defect_description: str = Form(""),
    is_warranty: bool = Form(False),
    db: Session = Depends(get_db)
):
    repair_data = schemas.RepairCreate(
        product_id=product_id,
        shop_id=shop_id,
        serial_number=serial_number,
        defect_description=defect_description,
        is_warranty=is_warranty
    )
    crud.create_repair(db, repair_data)
    return RedirectResponse(url="/repairs", status_code=303)

@app.post("/repairs/{repair_id}/complete")
def complete_repair_web(
    request: Request,
    repair_id: int,
    actual_cost: float = Form(...),
    db: Session = Depends(get_db)
):
    from decimal import Decimal
    complete_data = schemas.RepairComplete(actual_cost=Decimal(str(actual_cost)))
    crud.complete_repair(db, repair_id, complete_data.actual_cost)
    return RedirectResponse(url="/repairs", status_code=303)

@app.get("/categories", response_class=HTMLResponse)
def categories_page(request: Request, db: Session = Depends(get_db)):
    categories = crud.get_categories(db)
    stats = services.get_category_statistics(db)
    return templates.TemplateResponse("categories.html", {
        "request": request,
        "categories": [cat[0] for cat in categories],
        "stats": stats
    })

def check_database_connection():
    try:
        db = SessionLocal()
        db.execute(text("SELECT 1"))
        db.close()
        return True
    except Exception as e:
        logger.error(f"❌ Проверка подключения к БД не удалась: {e}")
        return False

if __name__ == "__main__":
    if check_database_connection():
        logger.info("🚀 Запуск FastAPI приложения...")
        uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
    else:
        logger.error("❌ Не удалось подключиться к базе данных. Запуск отменен.")