#include "MatchingEngine.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

int main() {
    using namespace hft;
    using clock = std::chrono::steady_clock;

    constexpr std::uint64_t N = 1'000'000;

    MatchingEngine engine(2'000'000);
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> price_offset(-50, 50);

    // Warm-up.
    for (std::uint64_t i = 1; i <= 10'000; ++i) {
        const Side side = side_dist(rng) == 0 ? Side::Buy : Side::Sell;
        const Price price = 10000 + price_offset(rng);
        engine.submitLimitOrder(i, side, price, 1);
    }

    const auto start = clock::now();

    for (std::uint64_t i = 10'001; i <= N + 10'000; ++i) {
        const Side side = side_dist(rng) == 0 ? Side::Buy : Side::Sell;
        const Price price = 10000 + price_offset(rng);
        engine.submitLimitOrder(i, side, price, 1);
    }

    const auto end = clock::now();

    const auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    const double seconds = static_cast<double>(ns) / 1e9;
    const double throughput = static_cast<double>(N) / seconds;
    const double avg_ns = static_cast<double>(ns) / static_cast<double>(N);

    std::cout << "Orders: " << N << '\n';
    std::cout << "Total time: " << ns << " ns\n";
    std::cout << "Average time/order: " << avg_ns << " ns\n";
    std::cout << "Throughput: " << throughput << " orders/sec\n";

    std::cout << "\nNote: this benchmark reports aggregate/average latency only.\n"
                 "For a serious latency study, collect per-order timestamps and\n"
                 "report p50/p95/p99 on a controlled machine.\n";

    return 0;
}
