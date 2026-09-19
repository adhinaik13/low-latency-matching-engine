#include "MatchingEngine.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

int main(int argc, char* argv[]) {
    using namespace hft;
    using clock = std::chrono::steady_clock;

    constexpr std::uint64_t DEFAULT_N = 1'000'000;
    constexpr std::uint64_t DEFAULT_WARMUP = 10'000;
    constexpr std::uint64_t DEFAULT_SAMPLE_INTERVAL = 100;
    constexpr int DEFAULT_RUNS = 5;
    constexpr std::uint64_t CALIBRATION_SAMPLES = 10'000;

    const std::uint64_t N =
        argc > 1 ? std::stoull(argv[1]) : DEFAULT_N;

    const int RUNS =
        argc > 2 ? std::stoi(argv[2]) : DEFAULT_RUNS;

    const std::uint64_t SAMPLE_INTERVAL =
        argc > 3
            ? std::stoull(argv[3])
            : DEFAULT_SAMPLE_INTERVAL;

    if (N == 0 || RUNS <= 0 || SAMPLE_INTERVAL == 0) {
        std::cerr
            << "Usage: engine_benchmark.exe "
            << "[orders] [runs] [sample_interval]\n";
        return 1;
    }

    constexpr std::uint64_t WARMUP = DEFAULT_WARMUP;

    // Measure the cost of the timing pair itself.
    std::vector<std::uint64_t> clock_overheads;
    clock_overheads.reserve(CALIBRATION_SAMPLES);

    for (std::uint64_t i = 0;
         i < CALIBRATION_SAMPLES;
         ++i) {

        const auto start = clock::now();
        const auto end = clock::now();

        const auto elapsed =
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                end - start)
                .count();

        clock_overheads.push_back(
            static_cast<std::uint64_t>(elapsed));
    }

    std::sort(
        clock_overheads.begin(),
        clock_overheads.end());

    const auto clock_percentile =
        [&](double p) -> std::uint64_t {
            const std::size_t index =
                static_cast<std::size_t>(
                    p * clock_overheads.size());

            return clock_overheads[
                std::min(
                    index,
                    clock_overheads.size() - 1)];
        };

    const std::uint64_t clock_overhead_p50 =
        clock_percentile(0.50);

    const std::uint64_t clock_overhead_p95 =
        clock_percentile(0.95);

    std::vector<double> throughputs;
    throughputs.reserve(RUNS);

    std::vector<std::uint64_t> final_latencies;

    for (int run = 1; run <= RUNS; ++run) {
        MatchingEngine engine(2'000'000);

        // Use the same deterministic workload for every run.
        std::mt19937_64 rng(42);
        std::uniform_int_distribution<int> side_dist(0, 1);
        std::uniform_int_distribution<int> price_offset(-50, 50);

        // Warm-up.
        for (std::uint64_t i = 1; i <= WARMUP; ++i) {
            const Side side =
                side_dist(rng) == 0
                    ? Side::Buy
                    : Side::Sell;

            const Price price =
                10000 + price_offset(rng);

            engine.submitLimitOrder(
                i, side, price, 1);
        }

        std::vector<std::uint64_t> latencies;

        if (run == RUNS) {
            latencies.reserve(
                N / SAMPLE_INTERVAL + 1);
        }

        const auto total_start = clock::now();

        for (std::uint64_t i = WARMUP + 1;
             i <= N + WARMUP;
             ++i) {

            const Side side =
                side_dist(rng) == 0
                    ? Side::Buy
                    : Side::Sell;

            const Price price =
                10000 + price_offset(rng);

            if (run == RUNS &&
                i % SAMPLE_INTERVAL == 0) {

                const auto start = clock::now();

                engine.submitLimitOrder(
                    i, side, price, 1);

                const auto end = clock::now();

                const auto latency =
                    std::chrono::duration_cast<
                        std::chrono::nanoseconds>(
                        end - start)
                        .count();

                latencies.push_back(
                    static_cast<std::uint64_t>(latency));

            } else {
                engine.submitLimitOrder(
                    i, side, price, 1);
            }
        }

        const auto total_end = clock::now();

        const auto total_ns =
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                total_end - total_start)
                .count();

        const double seconds =
            static_cast<double>(total_ns) / 1e9;

        const double throughput =
            static_cast<double>(N) / seconds;

        throughputs.push_back(throughput);

        std::cout << "Run " << run
                  << ": "
                  << std::fixed
                  << std::setprecision(2)
                  << throughput
                  << " orders/sec\n";

        if (run == RUNS) {
            final_latencies = std::move(latencies);
        }
    }

    const double min_throughput =
        *std::min_element(
            throughputs.begin(),
            throughputs.end());

    const double max_throughput =
        *std::max_element(
            throughputs.begin(),
            throughputs.end());

    double sum = 0.0;

    for (const double value : throughputs) {
        sum += value;
    }

    const double average_throughput =
        sum / static_cast<double>(
            throughputs.size());

    std::vector<double> sorted_throughputs =
        throughputs;

    std::sort(
        sorted_throughputs.begin(),
        sorted_throughputs.end());

    const double median_throughput =
        sorted_throughputs[
            sorted_throughputs.size() / 2];

    double squared_diff_sum = 0.0;

    for (const double value : throughputs) {
        const double diff =
            value - average_throughput;

        squared_diff_sum += diff * diff;
    }

    const double standard_deviation =
        std::sqrt(
            squared_diff_sum /
            static_cast<double>(throughputs.size()));

    const double coefficient_of_variation =
        (average_throughput > 0.0)
            ? (standard_deviation /
               average_throughput) * 100.0
            : 0.0;

    std::sort(
        final_latencies.begin(),
        final_latencies.end());

    const auto percentile =
        [&](double p) -> std::uint64_t {
            const std::size_t index =
                static_cast<std::size_t>(
                    p * final_latencies.size());

            return final_latencies[
                std::min(
                    index,
                    final_latencies.size() - 1)];
        };

    std::cout << "\nConfiguration:\n";

    std::cout << "Orders: "
              << N << '\n';

    std::cout << "Runs: "
              << RUNS << '\n';

    std::cout << "Sampling interval: 1/"
              << SAMPLE_INTERVAL
              << " orders\n";

    std::cout << "\nThroughput summary:\n";

    std::cout << "Minimum: "
              << min_throughput
              << " orders/sec\n";

    std::cout << "Maximum: "
              << max_throughput
              << " orders/sec\n";

    std::cout << "Average: "
              << average_throughput
              << " orders/sec\n";

    std::cout << "Median:  "
              << median_throughput
              << " orders/sec\n";

    std::cout << "Std dev: "
              << standard_deviation
              << " orders/sec\n";

    std::cout << "CV:      "
              << coefficient_of_variation
              << "%\n";

    std::cout << "\nClock measurement overhead:\n";

    std::cout << "p50: "
              << clock_overhead_p50
              << " ns\n";

    std::cout << "p95: "
              << clock_overhead_p95
              << " ns\n";

    std::cout << "\nSampled latency from final run:\n";

    std::cout << "Samples: "
              << final_latencies.size()
              << '\n';

    std::cout << "p50:   "
              << percentile(0.50)
              << " ns\n";

    std::cout << "p95:   "
              << percentile(0.95)
              << " ns\n";

    std::cout << "p99:   "
              << percentile(0.99)
              << " ns\n";

    std::cout << "p99.9: "
              << percentile(0.999)
              << " ns\n";

    return 0;
}