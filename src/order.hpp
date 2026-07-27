#pragma once

#include <cstddef>

enum class Side {
  NotSet,
  Buy,
  Sell,
};

struct Order {
  size_t id;
  size_t price;
  size_t quantity;
  Side side;

  explicit Order(size_t id_, size_t price_, size_t quantity_, Side side_) 
    : id(id_), price(price_), quantity(quantity_), side(side_) {}

  Order() : id(0), price(0), quantity(0), side(Side::NotSet) {}

  // Order& is a reference to an already exiting
  // object, not a reference to memory
  // Here id is copied directly..it's an int so it copyies the int
  // If it were a int* it would copy the address 
  Order(const Order& other) 
    : id(other.id), price(other.price), quantity(other.quantity), side(other.side) {}

  // Order&& is effectivaly a temp object that can get
  // destroyed after the move
  Order(Order&& other) noexcept
    : id(other.id), price(other.price), quantity(other.quantity), side(other.side) {}

  Order& operator=(const Order& other) {
    id = other.id;
    price = other.price;
    quantity = other.quantity;
    side = other.side;
    return *this;
  }

  Order& operator=(Order&& other) noexcept {
    id = other.id;
    price = other.price;
    quantity = other.quantity;
    side = other.side;
    return *this;
  }

  // Destructor - free memory
  ~Order() {}
};
