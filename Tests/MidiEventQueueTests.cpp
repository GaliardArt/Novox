#include "MidiEventQueue.h"
#include <iostream>
#include <string>
namespace {
int checks = 0, failures = 0;
void check(bool condition, const std::string& name) {
    ++checks;
    if (condition) std::cout << "[PASS] " << name << '\n';
    else { ++failures; std::cerr << "[FAIL] " << name << '\n'; }
}
void testEmptyQueue() {
    MidiEventQueue queue; MidiEvent event;
    check(queue.isEmpty(), "New queue starts empty");
    check(!queue.pop(event), "Pop on empty queue returns false");
}
void testFifoOrder() {
    MidiEventQueue queue;
    const MidiEvent a { MidiEvent::Type::noteOn, 60, 100, 8192, 2 };
    const MidiEvent b { MidiEvent::Type::pitchBend, 0, 0, 10240, 2 };
    const MidiEvent c { MidiEvent::Type::noteOff, 60, 0, 8192, 2 };
    const bool pushed = queue.push(a) && queue.push(b) && queue.push(c);
    MidiEvent x, y, z;
    const bool popped = queue.pop(x) && queue.pop(y) && queue.pop(z);
    check(pushed && popped, "Queue accepts note and pitch-bend events");
    check(x.type == a.type && x.note == a.note && x.velocity == a.velocity
       && x.channel == a.channel && y.type == b.type && y.value == b.value
       && y.channel == b.channel && z.type == c.type && z.note == c.note,
       "Event type, payload, channel, and FIFO order are preserved");
    check(queue.isEmpty(), "Queue is empty after draining");
}
void testCapacityAndRecovery() {
    MidiEventQueue queue; bool accepted = true;
    for (std::size_t i = 0; i < MidiEventQueue::capacity; ++i)
        accepted = queue.push({ MidiEvent::Type::noteOn, static_cast<int>(i % 128), 100 }) && accepted;
    const bool rejected = !queue.push({ MidiEvent::Type::pitchBend, 0, 0, 8192, 1 });
    MidiEvent event; std::size_t drained = 0;
    while (queue.pop(event)) ++drained;
    const bool recovers = queue.push({ MidiEvent::Type::noteOff, 60, 0 });
    check(accepted && rejected, "Queue enforces capacity without overwriting unread events");
    check(drained == MidiEventQueue::capacity && recovers, "Queue recovers after consumer drains pending events");
}
void testRepeatedWraparound() {
    MidiEventQueue queue; bool preserved = true;
    for (int i = 0; i < 4096; ++i) {
        const MidiEvent expected { i % 3 == 0 ? MidiEvent::Type::pitchBend
                                  : (i % 2 == 0 ? MidiEvent::Type::noteOn : MidiEvent::Type::noteOff),
                                  i % 128, static_cast<uint8_t>(i % 128), i % 16384, (i % 16) + 1 };
        if (!queue.push(expected)) { preserved = false; break; }
        MidiEvent actual;
        if (!queue.pop(actual) || actual.type != expected.type || actual.note != expected.note
            || actual.velocity != expected.velocity || actual.value != expected.value
            || actual.channel != expected.channel) { preserved = false; break; }
    }
    check(preserved && queue.isEmpty(), "Repeated cycles preserve all MIDI event types across ring wraparound");
}
}
int main() {
    std::cout << "NOVOX MIDI event queue regression tests\n";
    testEmptyQueue(); testFifoOrder(); testCapacityAndRecovery(); testRepeatedWraparound();
    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
