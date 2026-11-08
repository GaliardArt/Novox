#include "NovoxLookAndFeel.h"

const juce::Colour NovoxLookAndFeel::background { 0xff1c1c1e };
const juce::Colour NovoxLookAndFeel::panel { 0xff2c2c2e };
const juce::Colour NovoxLookAndFeel::surface { 0xff3a3a3c };
const juce::Colour NovoxLookAndFeel::border { 0xff48484a };
const juce::Colour NovoxLookAndFeel::textPrimary { 0xfff5f5f7 };
const juce::Colour NovoxLookAndFeel::textSecondary { 0xffa8a8ad };
const juce::Colour NovoxLookAndFeel::accent { 0xffe47b91 };
const juce::Colour NovoxLookAndFeel::accentHover { 0xfff09aad };
const juce::Colour NovoxLookAndFeel::teal { 0xff63c7c0 };
const juce::Colour NovoxLookAndFeel::success { 0xff78d6a8 };

NovoxLookAndFeel::NovoxLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, background);
    setColour(juce::Label::textColourId, textPrimary);
    setColour(juce::TextButton::buttonColourId, surface);
    setColour(juce::TextButton::buttonOnColourId, accent);
    setColour(juce::TextButton::textColourOffId, textPrimary);
    setColour(juce::TextButton::textColourOnId, textPrimary);
    setColour(juce::ComboBox::backgroundColourId, surface);
    setColour(juce::ComboBox::textColourId, textPrimary);
    setColour(juce::ComboBox::outlineColourId, border);
    setColour(juce::ComboBox::arrowColourId, accent);
    setColour(juce::Slider::trackColourId, accent);
    setColour(juce::Slider::thumbColourId, accentHover);
    setColour(juce::Slider::textBoxTextColourId, textPrimary);
    setColour(juce::Slider::textBoxBackgroundColourId, surface);
    setColour(juce::Slider::textBoxOutlineColourId, border);
    setColour(juce::TextEditor::backgroundColourId, surface);
    setColour(juce::TextEditor::textColourId, textPrimary);
    setColour(juce::TextEditor::outlineColourId, border);
    setColour(juce::TextEditor::focusedOutlineColourId, accent);
    setColour(juce::ToggleButton::textColourId, textPrimary);
}

void NovoxLookAndFeel::applyTo(juce::Component& component)
{
    component.setLookAndFeel(this);
}

void NovoxLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& baseColour, bool highlighted, bool down)
{
    auto colour = baseColour;
    if (!button.isEnabled())
        colour = surface.withAlpha(0.45f);
    else if (down)
        colour = accent.darker(0.12f);
    else if (highlighted)
        colour = button.getToggleState() ? accentHover : surface.brighter(0.12f);

    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(colour);
    g.fillRoundedRectangle(bounds, 8.0f);
    if (!button.getToggleState() && !down)
    {
        g.setColour(border.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    }
}

void NovoxLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
    bool down, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float>(0.5f, 0.5f,
        static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f);
    g.setColour(down ? surface.brighter(0.12f) : surface);
    g.fillRoundedRectangle(bounds, 7.0f);
    g.setColour(box.hasKeyboardFocus(true) ? accent : border);
    g.drawRoundedRectangle(bounds, 7.0f, 1.0f);
    juce::Path arrow;
    const float cx = static_cast<float>(width) - 18.0f;
    const float cy = static_cast<float>(height) * 0.5f;
    arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
    arrow.lineTo(cx, cy + 2.5f);
    arrow.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(accent);
    g.strokePath(arrow, juce::PathStrokeType(1.7f, juce::PathStrokeType::JointStyle::curved));
}

void NovoxLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width,
    int height, float sliderPos, float, float, const juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float left = static_cast<float>(x) + 2.0f;
        const float right = static_cast<float>(x + width) - 2.0f;
        g.setColour(surface);
        g.fillRoundedRectangle(left, cy - 2.5f, right - left, 5.0f, 2.5f);
        g.setColour(accent);
        g.fillRoundedRectangle(left, cy - 2.5f, juce::jmax(0.0f, sliderPos - left), 5.0f, 2.5f);
        g.setColour(textPrimary);
        g.fillEllipse(sliderPos - 7.0f, cy - 7.0f, 14.0f, 14.0f);
        g.setColour(accent);
        g.drawEllipse(sliderPos - 7.0f, cy - 7.0f, 14.0f, 14.0f, 2.0f);
        return;
    }
    LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
}

void NovoxLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
    bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const float cy = bounds.getCentreY();
    const juce::Rectangle<float> track(bounds.getX(), cy - 10.0f, 36.0f, 20.0f);
    g.setColour(button.getToggleState() ? accent : surface.brighter(0.1f));
    g.fillRoundedRectangle(track, 10.0f);
    const float knobX = button.getToggleState() ? track.getRight() - 17.0f : track.getX() + 3.0f;
    g.setColour(textPrimary);
    g.fillEllipse(knobX, cy - 7.0f, 14.0f, 14.0f);
    g.setColour(highlighted ? accentHover : textSecondary);
    g.setFont(juce::FontOptions(14.0f));
    g.drawText(button.getButtonText(), 48, 0, juce::jmax(0, button.getWidth() - 48),
        button.getHeight(), juce::Justification::centredLeft, true);
}

void NovoxLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
    juce::TextEditor& editor)
{
    g.setColour(editor.hasKeyboardFocus(true) ? accent : border);
    g.drawRoundedRectangle(0.5f, 0.5f, static_cast<float>(width) - 1.0f,
        static_cast<float>(height) - 1.0f, 7.0f, 1.0f);
}
