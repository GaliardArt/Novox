#pragma once
#include "VocalPercussionDetector.h"
#include <array>
#include <atomic>
#include <cstddef>
class PercussionEventQueue {
public:
    static constexpr size_t capacity=64;
    bool push(const VocalPercussionResult& event) noexcept {
        const auto write=writeIndex.load(std::memory_order_relaxed);
        const auto next=increment(write);
        if(next==readIndex.load(std::memory_order_acquire)){dropped.fetch_add(1,std::memory_order_relaxed);return false;}
        events[write]=event; writeIndex.store(next,std::memory_order_release); return true;
    }
    bool pop(VocalPercussionResult& event) noexcept {
        const auto read=readIndex.load(std::memory_order_relaxed);
        if(read==writeIndex.load(std::memory_order_acquire)) return false;
        event=events[read]; readIndex.store(increment(read),std::memory_order_release); return true;
    }
    void reset() noexcept {readIndex.store(0,std::memory_order_relaxed);writeIndex.store(0,std::memory_order_relaxed);dropped.store(0,std::memory_order_relaxed);}
    uint64_t getDropped() const noexcept{return dropped.load(std::memory_order_relaxed);}
private:
    static constexpr size_t increment(size_t value) noexcept{return (value+1)%capacity;}
    std::array<VocalPercussionResult,capacity> events {};
    alignas(64) std::atomic<size_t> writeIndex{0};
    alignas(64) std::atomic<size_t> readIndex{0};
    std::atomic<uint64_t> dropped{0};
};
