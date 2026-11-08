#include "MidiRouter.h"
MidiRouter::MidiRouter() : juce::Thread("NOVOX MIDI Dispatch") {}
MidiRouter::~MidiRouter() { closeOutput(); }
juce::StringArray MidiRouter::getOutputNames() const {
    juce::StringArray names;
    const auto available = juce::MidiOutput::getAvailableDevices();
    for (const auto& device : available) names.add(device.name);
    return names;
}
bool MidiRouter::openOutput(int index) {
    closeOutput();
    devices = juce::MidiOutput::getAvailableDevices();
    if (!juce::isPositiveAndBelow(index, devices.size())) return false;
    output = juce::MidiOutput::openDevice(devices.getReference(index).identifier);
    if (output == nullptr) return false;
    droppedEvents.store(0, std::memory_order_relaxed);
    eventQueue.reset();
    if (!startThread(juce::Thread::Priority::normal)) {
        output.reset();
        return false;
    }
    return true;
}
void MidiRouter::closeOutput() {
    if (isThreadRunning()) {
        signalThreadShouldExit();
        notify();
        // Drain pending events and wait for a clean exit before releasing the driver.
        stopThread(-1);
    }
    eventQueue.reset();
    output.reset();
}
void MidiRouter::noteOn(int note, juce::uint8 velocity) {
    if (output != nullptr && juce::isPositiveAndBelow(note, 128))
        enqueue({ MidiEvent::Type::noteOn, note, velocity });
}
void MidiRouter::noteOff(int note) {
    if (output != nullptr && juce::isPositiveAndBelow(note, 128))
        enqueue({ MidiEvent::Type::noteOff, note, 0 });
}
void MidiRouter::enqueue(const MidiEvent& event) noexcept {
    if (!eventQueue.push(event)) {
        droppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    notify();
}
void MidiRouter::run() {
    for (;;) {
        MidiEvent event;
        if (eventQueue.pop(event)) {
            if (output != nullptr) {
                const auto message = event.type == MidiEvent::Type::noteOn
                    ? juce::MidiMessage::noteOn(1, event.note, event.velocity)
                    : juce::MidiMessage::noteOff(1, event.note);
                output->sendMessageNow(message);
            }
            continue;
        }
        if (threadShouldExit()) break;
        wait(-1);
    }
}
