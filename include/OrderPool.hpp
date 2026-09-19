#pragma once

#include "Order.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace hft {

// Simple reusable object pool.
// It is intentionally single-threaded: the matching engine owns the order book
// and pool. This avoids claiming thread safety where none is provided.
class OrderPool {
public:
    explicit OrderPool(std::size_t initial_capacity = 1024);

    Order* acquire(OrderId id,
                   Side side,
                   Price price,
                   Quantity quantity,
                   std::uint64_t sequence);

    void release(Order* order);

    std::size_t capacity() const noexcept { return storage_.size(); }
    std::size_t free_count() const noexcept { return free_.size(); }

private:
    std::vector<std::unique_ptr<Order>> storage_;
    std::vector<Order*> free_;
};

} // namespace hft
