#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

struct MidiEvent
{
    enum class Type : uint8_t { noteOn, noteOff, pitchBend };

    Type type = Type::noteOff;
    int note = 0;
    uint8_t velocity = 0;
    int value = 8192;
    int channel = 1;
};

class MidiEventQueue
{
public:
    static constexpr std::size_t capacity = 256;

    bool push(const MidiEvent& event) noexcept
    {
        const auto write = writeIndex.load(std::memory_order_relaxed);
        const auto read = readIndex.load(std::memory_order_acquire);
        if (write - read >= capacity)
            return false;

        events[write % capacity] = event;
        writeIndex.store(write + 1, std::memory_order_release);
        return true;
    }

    bool pop(MidiEvent& event) noexcept
    {
        const auto read = readIndex.load(std::memory_order_relaxed);
        const auto write = writeIndex.load(std::memory_order_acquire);
        if (read == write)
            return false;

        event = events[read % capacity];
        readIndex.store(read + 1, std::memory_order_release);
        return true;
    }

    bool isEmpty() const noexcept
    {
        return readIndex.load(std::memory_order_acquire)
            == writeIndex.load(std::memory_order_acquire);
    }

    void reset() noexcept
    {
        // Call only after the producer and consumer have stopped.
        readIndex.store(0, std::memory_order_relaxed);
        writeIndex.store(0, std::memory_order_relaxed);
    }

private:
    std::array<MidiEvent, capacity> events {};
    alignas(64) std::atomic<std::size_t> readIndex { 0 };
    alignas(64) std::atomic<std::size_t> writeIndex { 0 };
};
