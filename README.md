# Low-Latency Limit Order Book & Matching Engine

A C++20 educational trading-engine project designed around HFT-style order
matching, deterministic execution, concurrency, and low-latency systems concepts.

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
- Bounded lock-free SPSC queue
- Producer/consumer market-event pipeline
- Multi-run throughput benchmark
- CMake build

## Architecture

```text
Market Event Producer
        |
        v
   SPSC Queue
        |
        v
Market Event Consumer
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

The matching engine itself is intentionally single-threaded. A bounded
single-producer/single-consumer queue connects the simulated market-event
producer and consumer. This separates the event transport path from the
deterministic matching logic while keeping the core engine simple.

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

The test suite covers matching behavior, partial fills, cancellation,
order-book operations, the SPSC queue, and concurrent producer/consumer
correctness.

## Run benchmark

### Matching engine benchmark

```bash
./build/engine_benchmark
```

This benchmark measures aggregate order-processing throughput for the matching
engine.

### Event pipeline benchmark

```bash
./build/event_simulator
```

The event simulator accepts optional event-count and run-count arguments:

```bash
./build/event_simulator 5000000 5
```

The benchmark measures the wall-clock time for a producer/consumer pipeline
that generates market events, transfers them through the bounded SPSC queue,
and processes them through the matching engine.

For one measured Windows/MSYS2 Release environment, 5 million events per run
across 5 runs produced:

- Median throughput: approximately 4.14 million events/sec
- Average throughput: approximately 4.14 million events/sec
- Minimum throughput: approximately 4.05 million events/sec
- Maximum throughput: approximately 4.19 million events/sec
- Coefficient of variation: 1.18%

These results are environment-dependent and can vary with CPU, compiler,
operating system, background load, and machine configuration. The throughput
figure is an aggregate pipeline measurement, not a per-order latency
distribution or a p50/p95/p99 latency measurement.

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

### SPSC queue

The event pipeline uses a bounded single-producer/single-consumer ring
buffer implemented with atomic head and tail indices. The queue avoids mutexes
and dynamic allocation in its push/pop operations.

### Important limitation

This is a learning project, not production exchange infrastructure.It does
not implement production-grade networking, persistence, risk controls,
sequence recovery, market-data protocols, hardware timestamping, NUMA
placement, or kernel bypass.
