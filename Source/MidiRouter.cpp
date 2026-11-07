#include "MidiRouter.h"

juce::StringArray MidiRouter::getOutputNames() const
{
    juce::StringArray names;
    const auto available = juce::MidiOutput::getAvailableDevices();
    for (const auto& device : available)
        names.add(device.name);
    return names;
}

bool MidiRouter::openOutput(int index)
{
    closeOutput();
    devices = juce::MidiOutput::getAvailableDevices();
    if (!juce::isPositiveAndBelow(index, devices.size()))
        return false;

    output = juce::MidiOutput::openDevice(devices.getReference(index).identifier);
    return output != nullptr;
}

void MidiRouter::closeOutput()
{
    output.reset();
}

void MidiRouter::noteOn(int note, juce::uint8 velocity)
{
    if (output != nullptr && juce::isPositiveAndBelow(note, 128))
        output->sendMessageNow(juce::MidiMessage::noteOn(1, note, velocity));
}

void MidiRouter::noteOff(int note)
{
    if (output != nullptr && juce::isPositiveAndBelow(note, 128))
        output->sendMessageNow(juce::MidiMessage::noteOff(1, note));
}
