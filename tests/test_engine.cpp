#include "MatchingEngine.hpp"
#include "SPSCQueue.hpp"

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

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

void test_spsc_queue() {
    SPSCQueue<int, 4> queue;

    assert(queue.empty());
    assert(!queue.full());

    // Capacity 4 uses one slot as a sentinel,
    // so 3 elements can be stored at once.
    assert(queue.push(10));
    assert(queue.push(20));
    assert(queue.push(30));

    assert(queue.full());
    assert(!queue.push(40));

    int value = 0;

    assert(queue.pop(value));
    assert(value == 10);

    assert(queue.pop(value));
    assert(value == 20);

    assert(queue.pop(value));
    assert(value == 30);

    assert(queue.empty());
    assert(!queue.pop(value));

    // Verify that the queue can be reused after becoming empty.
    assert(queue.push(40));
    assert(queue.pop(value));
    assert(value == 40);
}

void test_spsc_concurrent() {
    constexpr std::size_t queue_capacity = 1024;
    constexpr int total_items = 1'000'000;

    SPSCQueue<int, queue_capacity> queue;

    std::atomic<bool> producer_done{false};
    std::atomic<bool> test_failed{false};

    std::thread producer([&]() {
        for (int i = 0; i < total_items; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }

        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        int expected = 0;
        int value = 0;

        while (expected < total_items) {
            if (queue.pop(value)) {
                if (value != expected) {
                    test_failed.store(true, std::memory_order_release);
                    return;
                }

                ++expected;
            } else if (producer_done.load(std::memory_order_acquire)) {
                test_failed.store(true, std::memory_order_release);
                return;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    assert(!test_failed.load(std::memory_order_acquire));
    assert(queue.empty());
}

int main() {
    test_full_match();
    test_partial_match();
    test_price_time_priority();
    test_no_cross();
    test_cancel();
    test_spsc_queue();
    test_spsc_concurrent();

    std::cout << "All tests passed.\n";
    return 0;
}