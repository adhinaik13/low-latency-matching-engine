cmake_minimum_required(VERSION 3.16)
project(low_latency_matching_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

add_library(engine
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/OrderPool.cpp
)

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (MSVC)
    target_compile_options(engine PRIVATE /W4)
else()
    target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(exchange_demo src/main.cpp)
target_link_libraries(exchange_demo PRIVATE engine)

if (BUILD_TESTS)
    enable_testing()
    add_executable(engine_tests tests/test_engine.cpp)
    target_link_libraries(engine_tests PRIVATE engine)
    add_test(NAME engine_tests COMMAND engine_tests)
endif()

if (BUILD_BENCHMARKS)
    add_executable(engine_benchmark benchmarks/benchmark.cpp)
    target_link_libraries(engine_benchmark PRIVATE engine)

add_executable(event_simulator
    src/event_simulator.cpp
)

target_link_libraries(event_simulator PRIVATE engine)
endif()
