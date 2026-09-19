#pragma once

#include "Order.hpp"

#include <cstdint>

namespace hft {

enum class EventType : std::uint8_t {
    NewOrder,
    CancelOrder
};

struct MarketEvent {
    EventType type{EventType::NewOrder};

    OrderId order_id{0};
    Side side{Side::Buy};

    Price price{0};
    Quantity quantity{0};
};

} // namespace hft