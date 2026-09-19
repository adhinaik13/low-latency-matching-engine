#include "MatchingEngine.hpp"

#include <cassert>
#include <iostream>

using namespace hft;

void test_full_match() {
    MatchingEngine e;

    e.submitLimitOrder(1, Side::Sell, 10000, 5);
    auto trades = e.submitLimitOrder(2, Side::Buy, 10000, 5);

    assert(trades.size() == 1);
    assert(trades[0].price == 10000);
    assert(trades[0].quantity == 5);
    assert(e.book().find(1) == nullptr);
    assert(e.book().find(2) == nullptr);
}

void test_partial_match() {
    MatchingEngine e;

    e.submitLimitOrder(1, Side::Sell, 10000, 5);
    auto trades = e.submitLimitOrder(2, Side::Buy, 10000, 3);

    assert(trades.size() == 1);
    assert(trades[0].quantity == 3);

    const Order* remaining = e.book().find(1);
    assert(remaining != nullptr);
    assert(remaining->remaining == 2);
}

void test_price_time_priority() {
    MatchingEngine e;

    e.submitLimitOrder(1, Side::Sell, 10100, 2);
    e.submitLimitOrder(2, Side::Sell, 10000, 3);
    e.submitLimitOrder(3, Side::Sell, 10000, 4);

    auto trades = e.submitLimitOrder(4, Side::Buy, 10100, 5);

    assert(trades.size() == 2);
    assert(trades[0].resting_order_id == 2);
    assert(trades[0].price == 10000);
    assert(trades[0].quantity == 3);
    assert(trades[1].resting_order_id == 3);
    assert(trades[1].quantity == 2);
}

void test_no_cross() {
    MatchingEngine e;

    e.submitLimitOrder(1, Side::Sell, 10100, 5);
    auto trades = e.submitLimitOrder(2, Side::Buy, 10000, 5);

    assert(trades.empty());
    assert(e.book().find(2) != nullptr);
    assert(e.book().bestBid() == 10000);
    assert(e.book().bestAsk() == 10100);
}

void test_cancel() {
    MatchingEngine e;

    e.submitLimitOrder(1, Side::Buy, 10000, 5);
    assert(e.cancel(1));
    assert(!e.cancel(1));
    assert(e.book().find(1) == nullptr);
}

int main() {
    test_full_match();
    test_partial_match();
    test_price_time_priority();
    test_no_cross();
    test_cancel();

    std::cout << "All tests passed.\n";
    return 0;
}
