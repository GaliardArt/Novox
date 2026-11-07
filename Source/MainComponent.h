#pragma once
#include <JuceHeader.h>
#include "VoiceEngine.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    VoiceEngine engine;
    juce::Label title, status, noteLabel, frequencyLabel;
    juce::Slider gainSlider;
    juce::TextButton refreshButton { "Refresh devices" };
    juce::AudioDeviceManager deviceManager;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
