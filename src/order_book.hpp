#pragma once

#include <functional>
#include <map>
#include <vector>
#include "order.hpp"
#include "price_level.hpp"
#include "trade.hpp"

struct OrderBook {
  public:
    std::pair<bool, std::vector<Trade>> addOrder(Order o);
    std::vector<Trade> matchOrder(Order& o);
    void cancelOrder(size_t id);
    std::optional<size_t> bestBid();
    std::optional<size_t> bestAsk();

  private:
    PriceLevel* bestBid_();
    PriceLevel* bestAsk_();
    bool tryRecordOrder(Order o);
    std::map<size_t, std::pair<Side, PriceLevel*>> orders_;
    std::map<size_t, PriceLevel, std::greater<size_t>> bids_;
    std::map<size_t, PriceLevel> asks_;
};
