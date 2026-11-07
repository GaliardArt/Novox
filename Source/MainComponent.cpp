#include "MainComponent.h"
#include <cmath>

namespace
{
    juce::String noteNameFromFrequency(float hz)
    {
        if (hz <= 0.0f) return "--";
        const int midi = juce::roundToInt(69.0 + 12.0 * std::log2(hz / 440.0));
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F",
                                       "F#", "G", "G#", "A", "A#", "B" };
        return juce::String(names[(midi % 12 + 12) % 12]) + juce::String(midi / 12 - 1);
    }
}

MainComponent::MainComponent()
{
    title.setText("NOVOX", juce::dontSendNotification);
    title.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    status.setText("Choose an input device and sing a sustained note.", juce::dontSendNotification);
    status.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(status);

    noteLabel.setText("--", juce::dontSendNotification);
    noteLabel.setFont(juce::FontOptions(72.0f, juce::Font::bold));
    noteLabel.setJustificationType(juce::Justification::centred);
    noteLabel.setColour(juce::Label::textColourId, juce::Colour(0xff78e0b5));
    addAndMakeVisible(noteLabel);

    frequencyLabel.setText("0.0 Hz  |  confidence 0%", juce::dontSendNotification);
    frequencyLabel.setJustificationType(juce::Justification::centred);
    frequencyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(frequencyLabel);

    gainSlider.setRange(0.1, 4.0, 0.05);
    gainSlider.setValue(1.0);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
    gainSlider.onValueChange = [this] { engine.setInputGain(static_cast<float>(gainSlider.getValue())); };
    addAndMakeVisible(gainSlider);

    refreshButton.onClick = [this]
    {
        status.setText("Audio input is managed by the system audio device settings.", juce::dontSendNotification);
    };
    addAndMakeVisible(refreshButton);

    const auto error = deviceManager.initialise(1, 0, nullptr, true);
    if (error.isNotEmpty())
        status.setText("Audio device error: " + error, juce::dontSendNotification);
    else
        deviceManager.addAudioCallback(&engine);

    setSize(960, 620);
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    deviceManager.removeAudioCallback(&engine);
    deviceManager.closeAudioDevice();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff15191f));
    g.setColour(juce::Colour(0xff252d37));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(24.0f), 18.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(48);
    title.setBounds(area.removeFromTop(48));
    status.setBounds(area.removeFromTop(36));
    area.removeFromTop(60);
    noteLabel.setBounds(area.removeFromTop(130));
    frequencyLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(70);
    gainSlider.setBounds(area.removeFromTop(36).withSizeKeepingCentre(420, 36));
    refreshButton.setBounds(area.removeFromTop(44).withSizeKeepingCentre(180, 36));
}

void MainComponent::timerCallback()
{
    const auto p = engine.getLatestPitch();
    noteLabel.setText(p.voiced ? noteNameFromFrequency(p.frequencyHz) : "--", juce::dontSendNotification);
    frequencyLabel.setText(juce::String(p.frequencyHz, 1) + " Hz  |  confidence "
        + juce::String(p.confidence * 100.0f, 0) + "%", juce::dontSendNotification);
    if (p.voiced)
        status.setText("Voice detected", juce::dontSendNotification);
}
