#pragma once

#include <cstddef>

struct Trade {
public:
  const size_t buy_side_id;
  const size_t sell_side_id;
  const size_t trade_price;
  const size_t size;
  const size_t resting_remaining;

  explicit Trade(const size_t buy_side_id, const size_t sell_side_id,
                 const size_t trade_price, const size_t size,
                 const size_t resting_remaining)
      : buy_side_id(buy_side_id), sell_side_id(sell_side_id),
        trade_price(trade_price), size(size),
        resting_remaining(resting_remaining) {}
};
