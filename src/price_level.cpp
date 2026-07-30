#include "price_level.hpp"
#include <algorithm>
#include <cstdint>

bool PriceLevel::addOrder(Order *o) {
  if (level_ != o->price) {
    return false;
  }

  append(o);
  o->restingAt = this;

  return true;
}

uint16_t PriceLevel::matchOrder(Order &o, std::vector<Trade> &trades) {
  uint16_t tradeCount = 0;
  while (o.quantity > 0 && !empty()) {
    Order *in = head_;
    // if I am looking for 100 units (o)
    // and i have 200 unit avail (in)
    // we only need 100
    // if I am looking for 50 units (o)
    // and I have 10 unit avail (in)
    // I can only fill 10 units
    size_t amt = std::min(o.quantity, in->quantity);

    o.quantity -= amt;
    in->quantity -= amt;

    // capture the trade
    if (o.side == Side::Buy) {
      trades.push_back(Trade(o.id, in->id, level_, amt, in->quantity));
    } else {
      trades.push_back(Trade(in->id, o.id, level_, amt, in->quantity));
    }
    ++tradeCount;

    // completely filled, remove
    if (in->quantity == 0) {
      pop();
      in->restingAt = nullptr;
    }
  }
  return tradeCount;
}

bool PriceLevel::cancelOrder(Order *o) {
  remove(o);
  o->restingAt = nullptr;
  return empty();
}

void PriceLevel::append(Order *o) {
  if (head_ == nullptr || tail_ == nullptr) {
    head_ = o;
    tail_ = o;
    return;
  }

  o->prev = tail_;
  tail_->next = o;
  tail_ = o;
}

Order *PriceLevel::pop() {
  if (head_ == tail_) {
    Order *tmp = head_;
    head_ = nullptr;
    tail_ = nullptr;

    return tmp;
  }

  Order *tmp = head_;

  head_->next->prev = nullptr;
  head_ = head_->next;

  tmp->next = nullptr;
  tmp->prev = nullptr;

  return tmp;
}

Order *PriceLevel::dequeue() {
  if (head_ == tail_) {
    Order *tmp = tail_;
    head_ = nullptr;
    tail_ = nullptr;
    return tmp;
  }

  Order *tmp = tail_;
  tail_->prev->next = nullptr;
  tail_ = tail_->prev;
  tmp->next = nullptr;
  tmp->prev = nullptr;

  return tmp;
}

void PriceLevel::remove(Order *o) {
  if (head_ == nullptr || tail_ == nullptr) {
    return;
  }

  // 1 item
  if (head_ == tail_ && head_ == o) {
    head_ = nullptr;
    tail_ = nullptr;
    return;
  }

  // first item
  if (head_ == o) {
    pop();
    return;
  }

  // last item
  if (tail_ == o) {
    dequeue();
    return;
  }

  // middle
  Order *tmp = o->prev;
  o->prev->next = o->next;
  o->next->prev = tmp;

  o->next = nullptr;
  o->prev = nullptr;

  return;
}
