import models
import schemas
from typing import List
from sqlalchemy.orm import Session
from sqlalchemy import func, case, and_

def get_unprofitability_rating(db: Session, limit: int = 15) -> List[schemas.UnprofitabilityRating]:
    warranty_costs_case = case(
        (models.Repair.client_cost == 0, models.Repair.actual_repair_cost),
        else_=0
    )
    
    query = (
        db.query(
            models.Product.model_name,
            models.Manufacturer.name.label("manufacturer"),
            models.Product.category,
            func.count(models.Repair.repair_id).label("repair_count"),
            func.sum(models.Repair.actual_repair_cost).label("total_repair_cost"),
            func.round(func.avg(models.Repair.actual_repair_cost), 2).label("avg_repair_cost"),
            func.sum(warranty_costs_case).label("warranty_costs"),
            func.rank().over(
                order_by=(
                    func.count(models.Repair.repair_id).desc(),
                    func.sum(models.Repair.actual_repair_cost).desc()
                )
            ).label("problem_rank")
        )
        .select_from(models.Product)
        .join(models.Manufacturer, models.Product.manufacturer_id == models.Manufacturer.manufacturer_id)
        .join(models.Repair, models.Product.product_id == models.Repair.product_id)
        .filter(models.Repair.repair_id.isnot(None))
        .group_by(
            models.Product.product_id,
            models.Product.model_name,
            models.Manufacturer.name,
            models.Product.category
        )
        .order_by(
            func.count(models.Repair.repair_id).desc(),
            func.sum(models.Repair.actual_repair_cost).desc()
        )
        .limit(limit)
    )
    
    results = query.all()
    
    return [
        schemas.UnprofitabilityRating(
            model_name=row.model_name,
            manufacturer=row.manufacturer,
            category=row.category,
            repair_count=row.repair_count,
            total_repair_cost=row.total_repair_cost,
            avg_repair_cost=row.avg_repair_cost,
            warranty_costs=row.warranty_costs,
            problem_rank=row.problem_rank
        )
        for row in results
    ]

def get_city_coverage_report(db: Session) -> List[schemas.CityCoverage]:
    total_shops_subq = (
        db.query(
            models.City.city_id,
            models.City.name.label("city_name"),
            func.count(func.distinct(models.RepairShop.shop_id)).label("total_shops")
        )
        .outerjoin(models.RepairShop, models.City.city_id == models.RepairShop.city_id)
        .group_by(models.City.city_id, models.City.name)
        .subquery()
    )
    
    query = (
        db.query(
            total_shops_subq.c.city_name,
            func.coalesce(models.Product.category, 'Все категории').label("category"),
            total_shops_subq.c.total_shops,
            func.count(func.distinct(models.RepairShop.shop_id)).label("specialized_shops"),
            func.count(models.Repair.repair_id).label("repairs_done"),
            case(
                (total_shops_subq.c.total_shops > 0, 
                 func.round(100.0 * func.count(func.distinct(models.RepairShop.shop_id)) / total_shops_subq.c.total_shops, 1)),
                else_=0
            ).label("coverage_percentage")
        )
        .select_from(total_shops_subq)
        .outerjoin(models.RepairShop, total_shops_subq.c.city_id == models.RepairShop.city_id)
        .outerjoin(models.ShopSpecialization, models.RepairShop.shop_id == models.ShopSpecialization.shop_id)
        .outerjoin(models.Product, models.ShopSpecialization.product_id == models.Product.product_id)
        .outerjoin(
            models.Repair, 
            and_(
                models.RepairShop.shop_id == models.Repair.shop_id,
                models.ShopSpecialization.product_id == models.Repair.product_id
            )
        )
        .group_by(total_shops_subq.c.city_name, total_shops_subq.c.total_shops, models.Product.category)
        .order_by(total_shops_subq.c.city_name, models.Product.category)
    )
    
    results = query.all()
    
    return [
        schemas.CityCoverage(
            city_name=row.city_name,
            category=row.category,
            total_shops=row.total_shops,
            specialized_shops=row.specialized_shops,
            repairs_done=row.repairs_done,
            coverage_percentage=float(row.coverage_percentage) if row.coverage_percentage else 0.0
        )
        for row in results
    ]

def get_category_statistics(db: Session) -> List[schemas.CategoryStats]:
    query = (
        db.query(
            models.Product.category,
            func.count(models.Product.product_id).label("product_count"),
            func.count(models.Repair.repair_id).label("repair_count"),
            func.coalesce(func.sum(models.Repair.actual_repair_cost), 0).label("total_repair_cost"),
            func.coalesce(func.avg(models.Repair.actual_repair_cost), 0).label("avg_repair_cost")
        )
        .select_from(models.Product)
        .outerjoin(models.Repair, models.Product.product_id == models.Repair.product_id)
        .group_by(models.Product.category)
        .order_by(
            func.count(models.Repair.repair_id).desc(),
            func.sum(models.Repair.actual_repair_cost).desc()
        )
    )
    
    results = query.all()
    
    return [
        schemas.CategoryStats(
            category=row.category,
            product_count=row.product_count,
            repair_count=row.repair_count,
            total_repair_cost=row.total_repair_cost,
            avg_repair_cost=row.avg_repair_cost
        )
        for row in results
    ]