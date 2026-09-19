# Low-Latency Limit Order Book & Matching Engine

A C++20 educational trading-engine project designed around HFT-style order
matching and low-latency systems concepts.

## Features

- Limit orders
- Bid/ask order books
- Price-time priority
- Full and partial fills
- Order cancellation
- Order-ID lookup using `std::unordered_map`
- Fixed-point integer prices
- Reusable order object pool
- Unit tests
- Simple throughput benchmark
- CMake build

## Architecture

```text
Order Generator
      |
      v
MatchingEngine
      |
      +----> Bid Book
      |
      +----> Ask Book
      |
      v
   Trades
```

The matching engine is intentionally single-threaded. This is a useful starting
point for studying deterministic matching and latency before introducing
concurrency.

## Build

### Linux / macOS

Requirements:
- C++20 compiler
- CMake 3.16+

```bash
git clone <your-repository-url>
cd low-latency-matching-engine

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run demo

```bash
./build/exchange_demo
```

## Run tests

```bash
ctest --test-dir build --output-on-failure
```

## Run benchmark

```bash
./build/engine_benchmark
```

Run the benchmark in `Release` mode. Results depend heavily on CPU, compiler,
OS, background load, and machine configuration.

## Design notes

### Price-time priority

For a buy order, the engine consumes the lowest available ask first. For a
sell order, it consumes the highest available bid first. Within one price
level, earlier orders are consumed first.

### Fixed-point prices

Prices use integer ticks rather than floating point:

```text
100.25 -> 10025
```

This avoids floating-point comparison issues in matching logic.

### Order lookup

The order book maintains an `unordered_map<OrderId, Order*>` so that order
IDs can be located without scanning every price level.

### Object pool

Orders are allocated from a reusable pool. This reduces repeated allocation
and deallocation during order processing.

### Important limitation

This is a learning project, not production exchange infrastructure. It does
not implement production-grade networking, persistence, risk controls,
sequence recovery, market-data protocols, hardware timestamping, NUMA
placement, kernel bypass, or a production lock-free queue.

## Suggested next improvements

1. Add per-order latency measurement and p50/p95/p99 reporting.
2. Add order modification.
3. Add market orders.
4. Add a deterministic replay engine for historical order-flow data.
5. Add a producer/consumer market-data simulator.
6. Benchmark alternative data structures.
7. Profile with Linux `perf`.
8. Add sanitizers and static analysis.
9. Add a CSV trade/order log.
10. Compare allocation strategies.

## Resume direction

After implementing and benchmarking the project yourself, it can support bullets
such as:

- Developed a C++20 limit-order matching engine implementing price-time
  priority, partial fills, and order cancellation.
- Designed an in-memory bid/ask order book with hash-based order-ID lookup and
  a reusable object pool to reduce allocation overhead.
- Built a benchmark harness to measure order-processing throughput and latency
  under simulated order flow.

Only report benchmark numbers that you actually measure and reproduce.
