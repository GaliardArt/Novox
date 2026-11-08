#pragma once
#include <JuceHeader.h>

class NovoxLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    static const juce::Colour background;
    static const juce::Colour panel;
    static const juce::Colour surface;
    static const juce::Colour border;
    static const juce::Colour textPrimary;
    static const juce::Colour textSecondary;
    static const juce::Colour accent;
    static const juce::Colour accentHover;
    static const juce::Colour teal;
    static const juce::Colour success;

    NovoxLookAndFeel();
    void applyTo(juce::Component& component);

    void drawButtonBackground(juce::Graphics&, juce::Button&,
        const juce::Colour&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int,
        juce::ComboBox&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
        const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawTextEditorOutline(juce::Graphics&, int, int, juce::TextEditor&) override;
};
