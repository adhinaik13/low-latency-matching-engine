#pragma once

#include "OrderBook.hpp"

#include <vector>

namespace hft {

class MatchingEngine {
public:
    explicit MatchingEngine(std::size_t pool_capacity = 4096);

    // Process a limit order. Any executable quantity is matched immediately;
    // residual quantity is added to the book.
    std::vector<Trade> submitLimitOrder(OrderId id,
                                        Side side,
                                        Price price,
                                        Quantity quantity);

    bool cancel(OrderId id);

    const OrderBook& book() const noexcept { return book_; }

private:
    OrderBook book_;
};

} // namespace hft
