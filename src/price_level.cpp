#include "price_level.hpp"
#include <algorithm>

bool PriceLevel::addOrder(Order o) {
  if (level_ != o.price) {
    return false;
  }

  orders_.push_back(std::move(o));
  return true;
}

std::vector<Trade> PriceLevel::matchOrder(Order &o) {
  std::vector<Trade> trades;
  while (o.quantity > 0 && !orders_.empty()) {
    Order &in = orders_.front();
    // if I am looking for 100 units (o)
    // and i have 200 unit avail (in)
    // we only need 100
    // if I am looking for 50 units (o)
    // and I have 10 unit avail (in)
    // I can only fill 10 units
    size_t amt = std::min(o.quantity, in.quantity);

    o.quantity -= amt;
    in.quantity -= amt;

    // capture the trade
    if (o.side == Side::Buy) {
      trades.push_back(Trade(o.id, in.id, level_, amt, in.quantity));
    } else {
      trades.push_back(Trade(in.id, o.id, level_, amt, in.quantity));
    }

    // completely filled, remove
    if (in.quantity == 0) {
      orders_.pop_front();
    }
  }
  return trades;
}

void PriceLevel::cancelOrder(size_t id) {
  orders_.erase(std::remove_if(orders_.begin(), orders_.end(),
                               [id](const Order &o) { return o.id == id; }),
                orders_.end());
  return;
}
