#pragma once

#include "order.hpp"
#include "order_pool.hpp"
#include "price_level.hpp"
#include "trade.hpp"
#include <unordered_map>
#include <vector>

template <size_t Capacity, size_t MinLevel, size_t MaxLevel> struct OrderBook {
public:
  explicit OrderBook<Capacity, MinLevel, MaxLevel>(
      const std::array<char, 5> symbol, size_t minPrice, size_t maxPrice)
      : minPrice_(minPrice), maxPrice_(maxPrice), symbol_(symbol) {
    maxSpread_ = (MaxLevel - MinLevel) + 1;

    bids_.reserve(maxSpread_);
    asks_.reserve(maxSpread_);

    for (size_t i = 0; i < maxSpread_; ++i) {
      size_t level = i + MinLevel;
      bids_.emplace_back(PriceLevel(level));
      asks_.emplace_back(PriceLevel(level));
    }
  }

  std::pair<bool, std::vector<Trade>> addOrder(Order o) {
    if (o.price < minPrice_ || o.price > maxPrice_) {
      return std::pair<bool, std::vector<Trade>>(false, std::vector<Trade>());
    }

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

  std::vector<Trade> matchOrder(Order &o) {
    std::vector<Trade> trades;
    if (o.side == Side::Buy) {
      auto best = bestAsk_();
      while (o.quantity > 0 && best != nullptr && o.price >= best->getLevel()) {
        auto t = best->matchOrder(o);
        trades.reserve(trades.size() + t.size());
        for (auto &tr : t) {
          trades.push_back(std::move(tr));
          if (tr.resting_remaining == 0) {
            auto node = orders_.find(tr.sell_side_id);
            if (node != orders_.end()) {
              Order *tmp = node->second.first;
              orders_.erase(node);
              orderPool_.deallocate(tmp);
            }
          }
        }

        if (best->empty()) {
          resetBestAsk_();
        }

        best = bestAsk_();
      }
    } else {
      auto best = bestBid_();
      while (o.quantity > 0 && best != nullptr && o.price <= best->getLevel()) {
        auto t = best->matchOrder(o);

        trades.reserve(trades.size() + t.size());
        for (auto &tr : t) {
          trades.push_back(std::move(tr));
          if (tr.resting_remaining == 0) {
            auto node = orders_.find(tr.buy_side_id);
            if (node != orders_.end()) {
              Order *tmp = node->second.first;
              orders_.erase(node);
              orderPool_.deallocate(tmp);
            }
          }
        }

        if (best->empty()) {
          resetBestBid_();
        }
        best = bestBid_();
      }
    }

    return trades;
  }

  void cancelOrder(size_t id) {
    auto node = orders_.extract(id);
    if (node.empty()) {
      return;
    }

    Order *o = node.mapped().first;
    PriceLevel *pl = node.mapped().second;

    pl->cancelOrder(o);

    if (o->side == Side::Buy) {
      if (pl->empty()) {
        // delete from bids_
        resetBestBid_();
      }
    } else {
      if (pl->empty()) {
        // delete from asks_
        resetBestAsk_();
      }
    }

    orderPool_.deallocate(o);
    return;
  }

  std::optional<size_t> bestBid() {
    auto pl = bestBid_();
    if (pl == nullptr) {
      return std::optional<size_t>();
    }

    return std::optional<size_t>(pl->getLevel());
  }

  std::optional<size_t> bestAsk() {
    auto pl = bestAsk_();
    if (pl == nullptr) {
      return std::optional<size_t>();
    }

    return std::optional<size_t>(pl->getLevel());
  }

private:
  size_t minPrice_;
  size_t maxPrice_;
  size_t maxSpread_;
  OrderPool<Order, Capacity> orderPool_;
  std::unordered_map<size_t, std::pair<Order *, PriceLevel *>> orders_;

  std::optional<size_t> bestBidIdx_;
  std::optional<size_t> bestAskIdx_;
  std::vector<PriceLevel> bids_;
  std::vector<PriceLevel> asks_;

  const std::array<char, 5> symbol_;

  size_t priceToIdx(size_t price) { return price - MinLevel; }

  size_t idxToPrice(size_t idx) { return idx + MinLevel; }

  void resetBestBid_() {
    if (!bestBidIdx_.has_value()) {
      return;
    }

    if (!bids_.at(bestBidIdx_.value()).empty()) {
      return;
    }

    for (size_t i = bestBidIdx_.value();; --i) {
      if (!bids_.at(i).empty()) {
        bestBidIdx_ = i;
        return;
      }

      if (i == 0) {
        break;
      }
    }

    bestBidIdx_.reset();
  }

  void resetBestAsk_() {
    if (!bestAskIdx_.has_value()) {
      return;
    }

    if (!asks_.at(bestAskIdx_.value()).empty()) {
      return;
    }

    for (size_t i = bestAskIdx_.value(); i < maxSpread_; ++i) {
      if (!asks_.at(i).empty()) {
        bestAskIdx_ = i;
        return;
      }
    }

    bestAskIdx_.reset();
  }

  PriceLevel *bestBid_() {
    if (!bestBidIdx_.has_value()) {
      return nullptr;
    }

    return &bids_.at(bestBidIdx_.value());
  }

  PriceLevel *bestAsk_() {
    if (!bestAskIdx_.has_value()) {
      return nullptr;
    }

    return &asks_.at(bestAskIdx_.value());
  }

  bool tryRecordOrder(Order o) {
    if (o.quantity == 0) {
      return true;
    }

    if (o.price > maxPrice_) {
      return false;
    }

    if (o.price < minPrice_) {
      return false;
    }

    Order *order = orderPool_.allocate(o.id, o.price, o.quantity, o.side);
    if (order == nullptr) {
      return false;
    }

    // ensure we have a price level
    PriceLevel *level;
    size_t levelIdx = priceToIdx(o.price);
    if (o.side == Side::Buy) {
      PriceLevel *bestBid = bestBid_();
      level = &bids_.at(levelIdx);

      bool ok = level->addOrder(order);
      if (!ok) {
        orderPool_.deallocate(order);
        return false;
      }
      if (bestBid == nullptr || o.price > bestBid->getLevel()) {
        bestBidIdx_ = levelIdx;
      }
    } else {
      PriceLevel *bestAsk = bestAsk_();
      level = &asks_.at(levelIdx);

      bool ok = level->addOrder(order);
      if (!ok) {
        orderPool_.deallocate(order);
        return false;
      }
      if (bestAsk == nullptr || o.price < bestAsk->getLevel()) {
        bestAskIdx_ = levelIdx;
      }
    }

    // add the order to the orders map
    orders_.try_emplace(order->id, std::pair(order, level));
    return true;
  }
};
