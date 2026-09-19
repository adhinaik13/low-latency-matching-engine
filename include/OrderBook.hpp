#pragma once

#include "Order.hpp"
#include "OrderPool.hpp"

#include <cstddef>
#include <map>
#include <unordered_map>
#include <vector>

namespace hft {

class OrderBook {
public:
    explicit OrderBook(std::size_t pool_capacity = 4096);

    // Returns false if the order ID already exists or quantity is zero.
    bool addOrder(OrderId id, Side side, Price price, Quantity quantity);

    // Cancels an existing order. Returns false if the ID is unknown.
    bool cancelOrder(OrderId id);

    // Matches an incoming limit order against the opposite book.
    // The returned trades are in execution order.
    std::vector<Trade> match(OrderId incoming_id,
                             Side side,
                             Price price,
                             Quantity quantity);

    const Order* find(OrderId id) const noexcept;

    std::size_t order_count() const noexcept { return orders_.size(); }

    // Best prices from the current book.
    Price bestBid() const;
    Price bestAsk() const;

private:
    using Queue = std::vector<Order*>;
    using BidBook = std::map<Price, Queue, std::greater<Price>>;
    using AskBook = std::map<Price, Queue, std::less<Price>>;

    bool crosses(Side incoming_side, Price incoming_price, Price resting_price) const;
    void removeEmptyLevel(Side side, Price price);
    void eraseFromLevel(Side side, Price price, Order* order);

    BidBook bids_;
    AskBook asks_;
    std::unordered_map<OrderId, Order*> orders_;
    OrderPool pool_;
    std::uint64_t next_sequence_{1};
};

} // namespace hft
