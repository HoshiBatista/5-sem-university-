from typing import Optional, List
from datetime import date
from decimal import Decimal
from pydantic import BaseModel, validator

class ManufacturerBase(BaseModel):
    name: str
    contact_info: Optional[str] = None

class ManufacturerCreate(ManufacturerBase):
    pass

class Manufacturer(ManufacturerBase):
    manufacturer_id: int
    
    class Config:
        from_attributes = True

class CityBase(BaseModel):
    name: str

class CityCreate(CityBase):
    pass

class City(CityBase):
    city_id: int
    
    class Config:
        from_attributes = True

class ProductBase(BaseModel):
    model_name: str
    category: str
    warranty_period: int
    base_repair_cost: Decimal

class ProductCreate(ProductBase):
    manufacturer_id: int

class Product(ProductBase):
    product_id: int
    manufacturer_id: int
    manufacturer: Manufacturer
    
    class Config:
        from_attributes = True

class RepairShopBase(BaseModel):
    address: str
    contact_phone: Optional[str] = None

class RepairShopCreate(RepairShopBase):
    city_id: int

class RepairShop(RepairShopBase):
    shop_id: int
    city_id: int
    city: City
    specializations: List[str] = []
    
    class Config:
        from_attributes = True

class RepairBase(BaseModel):
    serial_number: Optional[str] = None
    defect_description: Optional[str] = None

class RepairCreate(RepairBase):
    product_id: int
    shop_id: int
    is_warranty: bool = False

class RepairComplete(BaseModel):
    actual_cost: Decimal

    @validator('actual_cost')
    def validate_cost(cls, v):
        if v < 0:
            raise ValueError('Стоимость не может быть отрицательной')
        return v

class Repair(RepairBase):
    repair_id: int
    product_id: int
    shop_id: int
    start_date: date
    end_date: Optional[date]
    actual_repair_cost: Decimal
    client_cost: Decimal
    product: Product
    shop: RepairShop
    
    class Config:
        from_attributes = True

class UnprofitabilityRating(BaseModel):
    model_name: str
    manufacturer: str
    category: str
    repair_count: int
    total_repair_cost: Decimal
    avg_repair_cost: Decimal
    warranty_costs: Decimal
    problem_rank: int
    
    class Config:
        from_attributes = True

class CityCoverage(BaseModel):
    city_name: str
    category: str
    total_shops: int
    specialized_shops: int
    repairs_done: int
    coverage_percentage: float
    
    class Config:
        from_attributes = True

class CategoryStats(BaseModel):
    category: str
    product_count: int
    repair_count: int
    total_repair_cost: Decimal
    avg_repair_cost: Decimal
    
    class Config:
        from_attributes = True
