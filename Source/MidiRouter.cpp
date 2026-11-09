#include "MidiRouter.h"
#include "PitchBend.h"
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

    {
        const juce::ScopedLock lock(producerLock);
        eventQueue.reset();
    }
    droppedEvents.store(0, std::memory_order_relaxed);
    emergencyAllNotesOff.store(false, std::memory_order_relaxed);
    if (!startThread(juce::Thread::Priority::high))
    {
        output.reset();
        return false;
    }

    acceptingEvents.store(true, std::memory_order_release);
    return true;
}

void MidiRouter::closeOutput()
{
    acceptingEvents.store(false, std::memory_order_release);
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        notify();
        stopThread(-1);
    }

    {
        const juce::ScopedLock lock(producerLock);
        eventQueue.reset();
    }

    sendAllNotesOff();
    output.reset();
    emergencyAllNotesOff.store(false, std::memory_order_relaxed);
}

void MidiRouter::setChannel(int channel) noexcept
{
    const int safeChannel = std::clamp(channel, 1, 16);
    if (safeChannel != midiChannel.load(std::memory_order_relaxed))
        emergencyAllNotesOff.store(true, std::memory_order_release);
    midiChannel.store(safeChannel, std::memory_order_relaxed);
    notify();
}

void MidiRouter::noteOn(int note, juce::uint8 velocity)
{
    if (juce::isPositiveAndBelow(note, 128))
        enqueue({ MidiEvent::Type::noteOn, note, velocity });
}

void MidiRouter::noteOff(int note)
{
    if (juce::isPositiveAndBelow(note, 128))
        enqueue({ MidiEvent::Type::noteOff, note, 0 });
}

void MidiRouter::pitchBend(int value)
{
    enqueue({ MidiEvent::Type::pitchBend, 0, 0, std::clamp(value, 0, 16383) });
}

void MidiRouter::enqueue(const MidiEvent& event) noexcept
{
    const juce::ScopedLock lock(producerLock);
    if (!acceptingEvents.load(std::memory_order_acquire))
        return;

    auto stampedEvent = event;
    stampedEvent.channel = midiChannel.load(std::memory_order_relaxed);
    if (!eventQueue.push(stampedEvent))
    {
        droppedEvents.fetch_add(1, std::memory_order_relaxed);
        if (event.type == MidiEvent::Type::noteOff)
            emergencyAllNotesOff.store(true, std::memory_order_release);
        notify();
        return;
    }
    notify();
}

void MidiRouter::sendAllNotesOff() noexcept
{
    if (output == nullptr)
        return;

    for (int channel = 1; channel <= 16; ++channel)
    {
        output->sendMessageNow(juce::MidiMessage::controllerEvent(channel, 123, 0));
        output->sendMessageNow(juce::MidiMessage::pitchWheel(channel, PitchBend::center));
    }
}

void MidiRouter::run()
{
    for (;;)
    {
        if (emergencyAllNotesOff.exchange(false, std::memory_order_acq_rel))
        {
            MidiEvent discarded;
            while (eventQueue.pop(discarded)) {}
            sendAllNotesOff();
            continue;
        }

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
