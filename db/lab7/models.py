from database import Base
from sqlalchemy.sql import func
from sqlalchemy.orm import relationship
from sqlalchemy import Column, Integer, String, Text, Date, Numeric, ForeignKey, CheckConstraint

class Manufacturer(Base):
    __tablename__ = "manufacturers"
    
    manufacturer_id = Column(Integer, primary_key=True, index=True)
    name = Column(String(100), nullable=False)
    contact_info = Column(Text)
    
    products = relationship("Product", back_populates="manufacturer")

class City(Base):
    __tablename__ = "cities"
    
    city_id = Column(Integer, primary_key=True, index=True)
    name = Column(String(50), nullable=False)
    
    shops = relationship("RepairShop", back_populates="city")

class Product(Base):
    __tablename__ = "products"
    
    product_id = Column(Integer, primary_key=True, index=True)
    manufacturer_id = Column(Integer, ForeignKey("manufacturers.manufacturer_id"), nullable=False)
    model_name = Column(String(100), nullable=False)
    category = Column(String(50), nullable=False)
    warranty_period = Column(Integer, nullable=False)
    base_repair_cost = Column(Numeric(10, 2), nullable=False)
    
    manufacturer = relationship("Manufacturer", back_populates="products")
    specializations = relationship("ShopSpecialization", back_populates="product")
    repairs = relationship("Repair", back_populates="product")
    
    __table_args__ = (
        CheckConstraint('warranty_period > 0', name='check_warranty_positive'),
        CheckConstraint('base_repair_cost >= 0', name='check_base_cost_non_negative'),
    )

class RepairShop(Base):
    __tablename__ = "repair_shops"
    
    shop_id = Column(Integer, primary_key=True, index=True)
    city_id = Column(Integer, ForeignKey("cities.city_id"), nullable=False)
    address = Column(String(255), nullable=False)
    contact_phone = Column(String(20))
    
    city = relationship("City", back_populates="shops")
    specializations = relationship("ShopSpecialization", back_populates="shop")
    repairs = relationship("Repair", back_populates="shop")

class ShopSpecialization(Base):
    __tablename__ = "shop_specializations"
    
    specialization_id = Column(Integer, primary_key=True, index=True)
    shop_id = Column(Integer, ForeignKey("repair_shops.shop_id"), nullable=False)
    product_id = Column(Integer, ForeignKey("products.product_id"), nullable=False)
    
    shop = relationship("RepairShop", back_populates="specializations")
    product = relationship("Product", back_populates="specializations")

class Repair(Base):
    __tablename__ = "repairs"
    
    repair_id = Column(Integer, primary_key=True, index=True)
    product_id = Column(Integer, ForeignKey("products.product_id"), nullable=False)
    shop_id = Column(Integer, ForeignKey("repair_shops.shop_id"), nullable=False)
    start_date = Column(Date, nullable=False, default=func.now())
    end_date = Column(Date)
    actual_repair_cost = Column(Numeric(10, 2), nullable=False)
    client_cost = Column(Numeric(10, 2), nullable=False)
    serial_number = Column(String(50))
    defect_description = Column(Text)
    
    product = relationship("Product", back_populates="repairs")
    shop = relationship("RepairShop", back_populates="repairs")
    
    __table_args__ = (
        CheckConstraint('end_date IS NULL OR end_date >= start_date', name='check_repair_dates'),
        CheckConstraint('actual_repair_cost >= 0', name='check_actual_cost_non_negative'),
        CheckConstraint('client_cost >= 0', name='check_client_cost_non_negative'),
    )