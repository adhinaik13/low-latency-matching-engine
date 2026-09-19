#include "MarketEvent.hpp"
#include "MatchingEngine.hpp"
#include "SPSCQueue.hpp"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>

using namespace hft;

int main() {
    constexpr std::size_t QueueCapacity = 1024;
    constexpr std::uint64_t EventCount = 100'000;

    SPSCQueue<MarketEvent, QueueCapacity> queue;

    std::atomic<bool> producer_done{false};
    std::atomic<std::uint64_t> processed_events{0};
    std::atomic<std::uint64_t> generated_trades{0};

    std::thread producer([&]() {
        for (std::uint64_t i = 1; i <= EventCount; ++i) {
            MarketEvent event;

            event.type = EventType::NewOrder;
            event.order_id = i;
            event.side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            event.price = 10000;
            event.quantity = 1;

            while (!queue.push(event)) {
                std::this_thread::yield();
            }
        }

        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        MatchingEngine engine;

        MarketEvent event;

        while (true) {
            if (queue.pop(event)) {
                if (event.type == EventType::NewOrder) {
                    auto trades = engine.submitLimitOrder(
                        event.order_id,
                        event.side,
                        event.price,
                        event.quantity
                    );

                    generated_trades.fetch_add(
                        trades.size(),
                        std::memory_order_relaxed
                    );
                } else if (event.type == EventType::CancelOrder) {
                    engine.cancel(event.order_id);
                }

                processed_events.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            } else if (producer_done.load(std::memory_order_acquire)) {
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    std::cout << "Events generated:  " << EventCount << '\n';
    std::cout << "Events processed: " << processed_events.load() << '\n';
    std::cout << "Trades generated: " << generated_trades.load() << '\n';

    if (processed_events.load() != EventCount) {
        std::cerr << "ERROR: not all events were processed.\n";
        return 1;
    }

    std::cout << "Event pipeline completed successfully.\n";

    return 0;
}