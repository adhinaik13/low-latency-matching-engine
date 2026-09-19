#include "MarketEvent.hpp"
#include "MatchingEngine.hpp"
#include "SPSCQueue.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>

using namespace hft;

int main() {
    constexpr std::size_t QueueCapacity = 1024;
    constexpr std::uint64_t EventCount = 1'000'000;

    SPSCQueue<MarketEvent, QueueCapacity> queue;

    std::atomic<bool> producer_done{false};
    std::atomic<std::uint64_t> processed_events{0};
    std::atomic<std::uint64_t> generated_trades{0};

    const auto start = std::chrono::steady_clock::now();

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

    const auto end = std::chrono::steady_clock::now();

    const auto elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        ).count();

    const double elapsed_seconds =
        static_cast<double>(elapsed_ns) / 1'000'000'000.0;

    const double events_per_second =
        static_cast<double>(EventCount) / elapsed_seconds;

    const double average_ns_per_event =
        static_cast<double>(elapsed_ns) /
        static_cast<double>(EventCount);

    const auto processed = processed_events.load();
    const auto trades = generated_trades.load();

    std::cout << "Events generated:  " << EventCount << '\n';
    std::cout << "Events processed: " << processed << '\n';
    std::cout << "Trades generated: " << trades << '\n';

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Elapsed time:      "
              << elapsed_seconds << " s\n";
    std::cout << "Throughput:        "
              << events_per_second << " events/sec\n";
    std::cout << "Average/event:     "
              << average_ns_per_event << " ns\n";

    if (processed != EventCount) {
        std::cerr << "ERROR: not all events were processed.\n";
        return 1;
    }

    if (trades != EventCount / 2) {
        std::cerr << "ERROR: unexpected trade count.\n";
        return 1;
    }

    std::cout << "Event pipeline completed successfully.\n";

    return 0;
}