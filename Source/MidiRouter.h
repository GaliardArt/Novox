#pragma once
#include <JuceHeader.h>

class MidiRouter
{
public:
    juce::StringArray getOutputNames() const;
    bool openOutput(int index);
    void closeOutput();
    bool isOpen() const noexcept { return output != nullptr; }
    void noteOn(int note, juce::uint8 velocity = 100);
    void noteOff(int note);

private:
    juce::Array<juce::MidiDeviceInfo> devices;
    std::unique_ptr<juce::MidiOutput> output;
};
