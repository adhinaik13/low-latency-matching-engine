#include "MatchingEngine.hpp"

#include <iostream>

int main() {
    using namespace hft;

    MatchingEngine engine;

    // Resting sell: 5 units at 100.00.
    engine.submitLimitOrder(1, Side::Sell, 10000, 5);

    // Buy 3 units at 100.00 -> executes 3.
    auto trades = engine.submitLimitOrder(2, Side::Buy, 10000, 3);

    for (const auto& trade : trades) {
        std::cout << "Trade: aggressive=" << trade.aggressive_order_id
                  << " resting=" << trade.resting_order_id
                  << " price=" << trade.price / 100.0
                  << " quantity=" << trade.quantity << '\n';
    }

    // Buy 4 units at 99.00 does not cross the 100.00 ask, so it rests.
    engine.submitLimitOrder(3, Side::Buy, 9900, 4);

    std::cout << "Best bid: " << engine.book().bestBid() / 100.0 << '\n';
    std::cout << "Best ask: " << engine.book().bestAsk() / 100.0 << '\n';

    // Cancel the resting buy.
    std::cout << "Cancel order 3: "
              << (engine.cancel(3) ? "success" : "not found") << '\n';

    return 0;
}
