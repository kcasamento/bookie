#pragma once

#include <cstddef>

enum class Side {
  NotSet,
  Buy,
  Sell,
};

struct Order {
public:
  Order *next;
  Order *prev;
  size_t id;
  size_t price;
  size_t quantity;
  Side side;

  explicit Order(size_t id_, size_t price_, size_t quantity_, Side side_)
      : next(nullptr), prev(nullptr), id(id_), price(price_),
        quantity(quantity_), side(side_) {}

  Order()
      : next(nullptr), prev(nullptr), id(0), price(0), quantity(0),
        side(Side::NotSet) {}

  // Order& is a reference to an already exiting
  // object, not a reference to memory
  // Here id is copied directly..it's an int so it copyies the int
  // If it were a int* it would copy the address
  // Order(const Order &other)
  //     : id(other.id), price(other.price), quantity(other.quantity),
  //       side(other.side) {}
  //
  Order(const Order &other) = delete;

  // Order&& is effectivaly a temp object that can get
  // destroyed after the move
  Order(Order &&other) noexcept
      : next(nullptr), prev(nullptr), id(other.id), price(other.price),
        quantity(other.quantity), side(other.side) {}

  // Order &operator=(const Order &other) {
  //   id = other.id;
  //   price = other.price;
  //   quantity = other.quantity;
  //   side = other.side;
  //   return *this;
  // }
  Order &operator=(const Order &other) = delete;

  Order &operator=(Order &&other) noexcept {
    id = other.id;
    price = other.price;
    quantity = other.quantity;
    side = other.side;
    next = nullptr;
    prev = nullptr;
    return *this;
  }
};
