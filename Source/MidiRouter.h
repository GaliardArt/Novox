#pragma once
#include <JuceHeader.h>
#include "MidiEventQueue.h"
class MidiRouter final : private juce::Thread {
public:
    MidiRouter();
    ~MidiRouter() override;
    juce::StringArray getOutputNames() const;
    bool openOutput(int index);
    void closeOutput();
    bool isOpen() const noexcept { return output != nullptr; }
    void noteOn(int note, juce::uint8 velocity = 100);
    void noteOff(int note);
    uint64_t getDroppedEvents() const noexcept { return droppedEvents.load(); }
private:
    void run() override;
    void enqueue(const MidiEvent& event) noexcept;
    juce::Array<juce::MidiDeviceInfo> devices;
    std::unique_ptr<juce::MidiOutput> output;
    MidiEventQueue eventQueue;
    std::atomic<uint64_t> droppedEvents { 0 };
};
