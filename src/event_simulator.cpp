#include "MarketEvent.hpp"
#include "MatchingEngine.hpp"
#include "SPSCQueue.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

using namespace hft;

int main(int argc, char* argv[]) {
    constexpr std::size_t QueueCapacity = 1024;

    std::uint64_t event_count = 1'000'000;
    int run_count = 5;

    if (argc >= 2) {
        event_count = std::stoull(argv[1]);
    }

    if (argc >= 3) {
        run_count = std::stoi(argv[2]);
    }

    if (event_count == 0 || run_count <= 0) {
        std::cerr << "Usage: event_simulator.exe [events] [runs]\n";
        return 1;
    }

    std::vector<double> throughputs;
    throughputs.reserve(run_count);

    for (int run = 1; run <= run_count; ++run) {
        SPSCQueue<MarketEvent, QueueCapacity> queue;

        std::atomic<bool> start_flag{false};
        std::atomic<bool> producer_done{false};

        std::atomic<std::uint64_t> processed_events{0};
        std::atomic<std::uint64_t> generated_trades{0};

        std::thread producer([&]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (std::uint64_t i = 1; i <= event_count; ++i) {
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

            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

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
                } else if (producer_done.load(
                               std::memory_order_acquire)) {
                    break;
                } else {
                    std::this_thread::yield();
                }
            }
        });

        // Both worker threads have been created before timing begins.
        const auto start = std::chrono::steady_clock::now();

        start_flag.store(true, std::memory_order_release);

        producer.join();
        consumer.join();

        const auto end = std::chrono::steady_clock::now();

        const auto elapsed_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                end - start
            ).count();

        const double elapsed_seconds =
            static_cast<double>(elapsed_ns) / 1'000'000'000.0;

        const double throughput =
            static_cast<double>(event_count) / elapsed_seconds;

        const double average_ns_per_event =
            static_cast<double>(elapsed_ns) /
            static_cast<double>(event_count);

        const auto processed = processed_events.load();
        const auto trades = generated_trades.load();

        if (processed != event_count) {
            std::cerr << "ERROR: run " << run
                      << " processed only "
                      << processed << " events.\n";
            return 1;
        }

        const std::uint64_t expected_trades = event_count / 2;

        if (trades != expected_trades) {
            std::cerr << "ERROR: run " << run
                      << " generated unexpected trade count: "
                      << trades << '\n';
            return 1;
        }

        throughputs.push_back(throughput);

        std::cout << "Run " << run
                  << ": throughput = "
                  << std::fixed << std::setprecision(2)
                  << throughput
                  << " events/sec, average = "
                  << average_ns_per_event
                  << " ns/event\n";
    }

    std::sort(throughputs.begin(), throughputs.end());

    const double sum =
        std::accumulate(throughputs.begin(), throughputs.end(), 0.0);

    const double average =
        sum / static_cast<double>(throughputs.size());

    const double median =
        (throughputs.size() % 2 == 0)
            ? (throughputs[throughputs.size() / 2 - 1] +
               throughputs[throughputs.size() / 2]) / 2.0
            : throughputs[throughputs.size() / 2];

    double squared_diff_sum = 0.0;

    for (double throughput : throughputs) {
        const double diff = throughput - average;
        squared_diff_sum += diff * diff;
    }

    const double standard_deviation =
        std::sqrt(
            squared_diff_sum /
            static_cast<double>(throughputs.size())
        );

    const double coefficient_of_variation =
        average > 0.0
            ? (standard_deviation / average) * 100.0
            : 0.0;

    std::cout << "\nSummary\n";
    std::cout << "-------\n";
    std::cout << "Events/run:       " << event_count << '\n';
    std::cout << "Runs:             " << run_count << '\n';
    std::cout << "Minimum:          " << throughputs.front()
              << " events/sec\n";
    std::cout << "Maximum:          " << throughputs.back()
              << " events/sec\n";
    std::cout << "Average:          " << average
              << " events/sec\n";
    std::cout << "Median:           " << median
              << " events/sec\n";
    std::cout << "Std dev:          " << standard_deviation
              << " events/sec\n";
    std::cout << "CV:               " << coefficient_of_variation
              << "%\n";

    std::cout << "\nEvent pipeline completed successfully.\n";

    return 0;
}