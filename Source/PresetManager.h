#pragma once

#include <juce_core/juce_core.h>
#include "PresetData.h"
#include <vector>

class PresetManager final
{
public:
    explicit PresetManager(juce::File presetDirectory = {});

    std::vector<juce::String> listPresets() const;
    bool savePreset(const juce::String& name, PresetData data, juce::String* error = nullptr) const;
    bool loadPreset(const juce::String& name, PresetData& data, juce::String* error = nullptr) const;
    bool deletePreset(const juce::String& name, juce::String* error = nullptr) const;

    const juce::File& getDirectory() const noexcept { return directory; }
    static juce::String sanitiseName(const juce::String& name);

private:
    juce::File fileForName(const juce::String& name) const;
    juce::File directory;
};
