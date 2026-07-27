#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>
#include <map>
#include "price_level.hpp"
#include "order_book.hpp"
#include "order.hpp"
#include "trade.hpp"

std::pair<bool, std::vector<Trade>> OrderBook::addOrder(Order o) {
  bool ok = true;
  std::vector<Trade> trades;
  if (o.side == Side::Buy) {
    auto best = bestAsk_();
    if (best != nullptr) {
      // try to match
      if (o.price >= best->getLevel()) {
        trades = matchOrder(o);
      }
    }
    
    ok = tryRecordOrder(std::move(o));
  } else {
    auto best = bestBid_();
    if (best != nullptr) {
      if (o.price <= best->getLevel()) {
        trades = matchOrder(o);
      }
    }

    ok = tryRecordOrder(std::move(o));
  }

  if (!ok) {
    return std::pair<bool, std::vector<Trade>>(false, std::vector<Trade>());
  }

    return std::pair<bool, std::vector<Trade>>(true, trades);
}

std::vector<Trade> OrderBook::matchOrder(Order& o) {
  std::vector<Trade> trades;
  if (o.side == Side::Buy) {
    auto best = bestAsk_();
    while(o.quantity > 0 && best != nullptr && o.price >= best->getLevel()) {
      auto t = best->matchOrder(o);
      std::move(t.begin(), t.end(), std::back_inserter(trades));
      
      if (best->empty()) {
        asks_.erase(asks_.begin());
      }

      best = bestAsk_();
    }
  } else {
    auto best = bestBid_();
    while(o.quantity > 0 && best != nullptr && o.price <= best->getLevel()) {
      auto t = best->matchOrder(o);

      trades.reserve(trades.size() + t.size());
      for(auto& tr : t) {
        trades.push_back(std::move(tr));
      }

      if (best->empty()) {
        bids_.erase(bids_.begin());
      }
      best = bestBid_();
    }
  }
  
  return trades;  
}

void OrderBook::cancelOrder(size_t id) {
  auto node = orders_.extract(id);
  if (node.empty()) {
    return;
  }

  node.mapped().second->cancelOrder(id);

  if (node.mapped().first == Side::Buy) {
    if (node.mapped().second->empty()) {
      // delete from bids_ 
      bids_.erase(node.mapped().second->getLevel());
    }
  } else {
    if (node.mapped().second->empty()) {
      // delete from asks_
      asks_.erase(node.mapped().second->getLevel());
    }
  }
  return;
}

std::optional<size_t> OrderBook::bestBid() {
  auto pl = bestBid_();
  if (pl == nullptr) {
    return std::optional<size_t>();
  }

  return std::optional<size_t>(pl->getLevel());
}

std::optional<size_t> OrderBook::bestAsk() {
  auto pl = bestAsk_();
  if (pl == nullptr) {
    return std::optional<size_t>();
  }

  return std::optional<size_t>(pl->getLevel());
}

PriceLevel* OrderBook::bestBid_() {
  if(bids_.empty()) {
    return nullptr;
  }

  return &bids_.begin()->second;
}

PriceLevel* OrderBook::bestAsk_() {
  if(asks_.empty()) {
    return nullptr;
  }

  return &asks_.begin()->second;
}

bool OrderBook::tryRecordOrder(Order o) {
  if (o.quantity == 0) {
    return true;
  }

  // ensure we have a price level
  PriceLevel* level;
  if (o.side == Side::Buy) {
    auto [it, inserted] = bids_.try_emplace(o.price, PriceLevel(o.price));
    level = &it->second;
  } else {
    auto [it, inserted] = asks_.try_emplace(o.price, PriceLevel(o.price));
    level = &it->second;
  }

  // add the order to the level
  size_t id = o.id;
  Side side = o.side;
  bool ok = level->addOrder(std::move(o));
  if (!ok) {
    return false;
  }

  // add the order to the orders map
  orders_.try_emplace(id, std::pair(side, level));
  return true;
}
