import models
import schemas
from typing import List
from sqlalchemy.orm import Session, joinedload
from sqlalchemy import func

def get_manufacturers(db: Session, skip: int = 0, limit: int = 100):
    return db.query(models.Manufacturer).offset(skip).limit(limit).all()

def get_cities(db: Session, skip: int = 0, limit: int = 100):
    return db.query(models.City).offset(skip).limit(limit).all()

def get_products(db: Session, skip: int = 0, limit: int = 100):
    return db.query(models.Product).options(
        joinedload(models.Product.manufacturer)
    ).offset(skip).limit(limit).all()

def get_product(db: Session, product_id: int):
    return db.query(models.Product).filter(models.Product.product_id == product_id).first()

def get_repair_shops(db: Session, skip: int = 0, limit: int = 100):
    shops = db.query(models.RepairShop).options(
        joinedload(models.RepairShop.city)
    ).offset(skip).limit(limit).all()
    
    for shop in shops:
        specializations = (
            db.query(models.Product.category)
            .join(models.ShopSpecialization)
            .filter(models.ShopSpecialization.shop_id == shop.shop_id)
            .distinct()
            .all()
        )
        shop.categories = [spec[0] for spec in specializations]
    
    return shops

def get_repair_shop(db: Session, shop_id: int):
    return db.query(models.RepairShop).filter(models.RepairShop.shop_id == shop_id).first()

def get_repairs(db: Session, skip: int = 0, limit: int = 100, active_only: bool = False):
    query = db.query(models.Repair).options(
        joinedload(models.Repair.product).joinedload(models.Product.manufacturer),
        joinedload(models.Repair.shop).joinedload(models.RepairShop.city)
    )
    if active_only:
        query = query.filter(models.Repair.end_date.is_(None))
    return query.offset(skip).limit(limit).all()

def get_repair(db: Session, repair_id: int):
    return db.query(models.Repair).filter(models.Repair.repair_id == repair_id).first()

def create_repair(db: Session, repair: schemas.RepairCreate):
    product = get_product(db, repair.product_id)
    if not product:
        return None
    
    base_cost = product.base_repair_cost
    actual_cost = 0 if repair.is_warranty else base_cost
    client_cost = 0 if repair.is_warranty else actual_cost
    
    db_repair = models.Repair(
        product_id=repair.product_id,
        shop_id=repair.shop_id,
        actual_repair_cost=actual_cost,
        client_cost=client_cost,
        serial_number=repair.serial_number,
        defect_description=repair.defect_description
    )
    
    db.add(db_repair)
    db.commit()
    db.refresh(db_repair)
    return db_repair

def complete_repair(db: Session, repair_id: int, actual_cost: float):
    repair = get_repair(db, repair_id)
    if not repair or repair.end_date is not None:
        return None
    
    repair.end_date = func.now()
    repair.actual_repair_cost = actual_cost
    if repair.client_cost > 0: 
        repair.client_cost = actual_cost
    
    db.commit()
    db.refresh(repair)
    return repair

def get_categories(db: Session):
    return db.query(models.Product.category).distinct().all()

def get_system_stats(db: Session):
    active_repairs = db.query(models.Repair).filter(models.Repair.end_date.is_(None)).count()
    completed_repairs = db.query(models.Repair).filter(models.Repair.end_date.isnot(None)).count()
    products_count = db.query(models.Product).count()
    shops_count = db.query(models.RepairShop).count()
    
    return {
        "active_repairs": active_repairs,
        "completed_repairs": completed_repairs,
        "products_count": products_count,
        "shops_count": shops_count
    }

def get_repairs_stats(db: Session):
    active_repairs_count = db.query(models.Repair).filter(models.Repair.end_date.is_(None)).count()
    completed_repairs_count = db.query(models.Repair).filter(models.Repair.end_date.isnot(None)).count()
    
    total_cost_result = db.query(func.sum(models.Repair.actual_repair_cost)).scalar()
    total_cost = float(total_cost_result) if total_cost_result else 0.0
    
    warranty_repairs = db.query(models.Repair).filter(models.Repair.client_cost == 0).count()
    
    return {
        "active_repairs": active_repairs_count,
        "completed_repairs": completed_repairs_count,
        "total_cost": total_cost,
        "warranty_repairs": warranty_repairs
    }