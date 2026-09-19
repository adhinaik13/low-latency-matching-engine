#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <utility>

template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert(Capacity > 0, "Capacity must be greater than zero");

public:
    SPSCQueue() = default;

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    bool push(const T& value) {
        const std::size_t current_tail =
            tail_.load(std::memory_order_relaxed);

        const std::size_t next_tail = increment(current_tail);

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[current_tail] = value;
        tail_.store(next_tail, std::memory_order_release);

        return true;
    }

    bool push(T&& value) {
        const std::size_t current_tail =
            tail_.load(std::memory_order_relaxed);

        const std::size_t next_tail = increment(current_tail);

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[current_tail] = std::move(value);
        tail_.store(next_tail, std::memory_order_release);

        return true;
    }

    bool pop(T& value) {
        const std::size_t current_head =
            head_.load(std::memory_order_relaxed);

        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }

        value = std::move(buffer_[current_head]);
        head_.store(increment(current_head), std::memory_order_release);

        return true;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    bool full() const {
        const std::size_t current_tail =
            tail_.load(std::memory_order_acquire);

        return increment(current_tail) ==
               head_.load(std::memory_order_acquire);
    }

private:
    static constexpr std::size_t increment(std::size_t index) {
        return (index + 1) % Capacity;
    }

    std::array<T, Capacity> buffer_{};

    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};