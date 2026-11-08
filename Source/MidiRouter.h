#pragma once
#include <JuceHeader.h>
#include "MidiEventQueue.h"

class MidiRouter final : private juce::Thread
{
public:
    MidiRouter();
    ~MidiRouter() override;

    juce::StringArray getOutputNames() const;
    bool openOutput(int index);
    void closeOutput();
    bool isOpen() const noexcept { return output != nullptr; }

    void setChannel(int channel) noexcept;
    int getChannel() const noexcept { return midiChannel.load(std::memory_order_relaxed); }
    void noteOn(int note, juce::uint8 velocity = 100);
    void noteOff(int note);
    void pitchBend(int value);
    void drumNoteOn(int note, juce::uint8 velocity = 100);
    void drumNoteOff(int note);
    uint64_t getDroppedEvents() const noexcept { return droppedEvents.load(); }

private:
    void run() override;
    void enqueue(const MidiEvent& event, bool percussionChannel = false) noexcept;

    juce::Array<juce::MidiDeviceInfo> devices;
    std::unique_ptr<juce::MidiOutput> output;
    MidiEventQueue eventQueue;
    std::atomic<int> midiChannel { 1 };
    std::atomic<uint64_t> droppedEvents { 0 };
};
