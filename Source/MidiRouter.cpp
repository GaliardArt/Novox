#include "MidiRouter.h"
#include <algorithm>

MidiRouter::MidiRouter() : juce::Thread("NOVOX MIDI Dispatch") {}
MidiRouter::~MidiRouter() { closeOutput(); }

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
    if (output == nullptr)
        return false;

    droppedEvents.store(0, std::memory_order_relaxed);
    eventQueue.reset();
    if (!startThread(juce::Thread::Priority::normal))
    {
        output.reset();
        return false;
    }
    return true;
}

void MidiRouter::closeOutput()
{
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        notify();
        // Drain pending events and wait for a clean exit before releasing the driver.
        stopThread(-1);
    }
    eventQueue.reset();
    output.reset();
}

void MidiRouter::setChannel(int channel) noexcept
{
    midiChannel.store(std::clamp(channel, 1, 16), std::memory_order_relaxed);
}

void MidiRouter::noteOn(int note, juce::uint8 velocity)
{
    if (output != nullptr && juce::isPositiveAndBelow(note, 128))
        enqueue({ MidiEvent::Type::noteOn, note, velocity });
}

void MidiRouter::noteOff(int note)
{
    if (output != nullptr && juce::isPositiveAndBelow(note, 128))
        enqueue({ MidiEvent::Type::noteOff, note, 0 });
}

void MidiRouter::pitchBend(int value)
{
    if (output != nullptr)
        enqueue({ MidiEvent::Type::pitchBend, 0, 0, std::clamp(value, 0, 16383) });
}

void MidiRouter::enqueue(const MidiEvent& event) noexcept
{
    auto stampedEvent = event;
    stampedEvent.channel = midiChannel.load(std::memory_order_relaxed);

    if (!eventQueue.push(stampedEvent))
    {
        droppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    notify();
}

void MidiRouter::run()
{
    for (;;)
    {
        MidiEvent event;
        if (eventQueue.pop(event))
        {
            if (output != nullptr)
            {
                juce::MidiMessage message;
                switch (event.type)
                {
                    case MidiEvent::Type::noteOn:
                        message = juce::MidiMessage::noteOn(event.channel, event.note, event.velocity);
                        break;
                    case MidiEvent::Type::noteOff:
                        message = juce::MidiMessage::noteOff(event.channel, event.note);
                        break;
                    case MidiEvent::Type::pitchBend:
                        message = juce::MidiMessage::pitchWheel(event.channel, event.value);
                        break;
                }
                output->sendMessageNow(message);
            }
            continue;
        }

        if (threadShouldExit())
            break;
        wait(-1);
    }
}
