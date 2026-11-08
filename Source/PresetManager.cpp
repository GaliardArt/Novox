#include "PresetManager.h"
#include <algorithm>
#include <utility>

namespace
{
    constexpr int presetSchemaVersion = 1;

    void setError(juce::String* error, const juce::String& message)
    {
        if (error != nullptr)
            *error = message;
    }

    juce::var readJson(const juce::File& file)
    {
        return juce::JSON::parse(file.loadFileAsString());
    }
}

PresetManager::PresetManager(juce::File presetDirectory)
    : directory(presetDirectory.getFullPathName().isEmpty()
        ? juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("NOVOX").getChildFile("Presets")
        : std::move(presetDirectory))
{
}

juce::String PresetManager::sanitiseName(const juce::String& input)
{
    const auto name = input.trim().substring(0, 48);
    juce::String result;
    for (int i = 0; i < name.length(); ++i)
    {
        const auto character = name[i];
        const auto code = static_cast<unsigned int>(character);
        if (code < 32 || character == '/' || character == '\\'
            || character == ':' || character == '*' || character == '?'
            || character == '"' || character == '<' || character == '>'
            || character == '|')
            continue;
        result += character;
    }
    return result.trim();
}

juce::File PresetManager::fileForName(const juce::String& name) const
{
    const auto safeName = sanitiseName(name);
    if (safeName.isEmpty())
        return {};
    return directory.getChildFile(safeName + ".novoxpreset");
}

std::vector<juce::String> PresetManager::listPresets() const
{
    std::vector<juce::String> names;
    if (!directory.isDirectory())
        return names;

    juce::Array<juce::File> files;
    directory.findChildFiles(files, juce::File::findFiles, false, "*.novoxpreset");
    for (const auto& file : files)
    {
        const auto parsed = readJson(file);
        if (auto* object = parsed.getDynamicObject())
        {
            const auto name = object->getProperty("name").toString().trim();
            if (!name.isEmpty())
                names.push_back(name);
        }
    }

    std::sort(names.begin(), names.end(), [](const auto& a, const auto& b)
    {
        return a.compareIgnoreCase(b) < 0;
    });
    return names;
}

bool PresetManager::savePreset(const juce::String& name, PresetData data, juce::String* error) const
{
    const auto safeName = sanitiseName(name);
    if (safeName.isEmpty())
    {
        setError(error, "Enter a valid preset name.");
        return false;
    }

    if (directory.createDirectory().failed())
    {
        setError(error, "Could not create the local presets directory.");
        return false;
    }

    data.sanitise();
    auto* object = new juce::DynamicObject();
    object->setProperty("schemaVersion", presetSchemaVersion);
    object->setProperty("name", safeName);
    object->setProperty("inputGain", data.inputGain);
    object->setProperty("pitchSensitivity", data.pitchSensitivity);
    object->setProperty("pitchSmoothing", data.pitchSmoothing);
    object->setProperty("pitchBendEnabled", data.pitchBendEnabled);
    object->setProperty("noteTransitionSmoothingEnabled", data.noteTransitionSmoothingEnabled);
    object->setProperty("noteTransitionStability", data.noteTransitionStability);
    object->setProperty("autoTuneEnabled",data.autoTuneEnabled); object->setProperty("autoTuneTonic",data.autoTuneTonic); object->setProperty("autoTuneScale",data.autoTuneScale);
    object->setProperty("noiseReductionEnabled", data.noiseReductionEnabled);
    object->setProperty("noiseReductionStrength", data.noiseReductionStrength);
    object->setProperty("pitchStabilityEnabled", data.pitchStabilityEnabled);
    object->setProperty("pitchDropToleranceCents", data.pitchDropToleranceCents);
    object->setProperty("pitchDropHoldFrames", data.pitchDropHoldFrames);
    object->setProperty("inputChannelMode", data.inputChannelMode);
    object->setProperty("pitchBendRange", data.pitchBendRange);
    object->setProperty("midiChannel", data.midiChannel);
    object->setProperty("midiOutputName", juce::String(data.midiOutputName));

    const auto json = juce::JSON::toString(juce::var(object), true);
    if (!fileForName(safeName).replaceWithText(json))
    {
        setError(error, "Could not write the preset file.");
        return false;
    }
    return true;
}

bool PresetManager::loadPreset(const juce::String& name, PresetData& data, juce::String* error) const
{
    if (sanitiseName(name).isEmpty())
    {
        setError(error, "The selected preset could not be found.");
        return false;
    }
    const auto file = fileForName(name);
    if (!file.existsAsFile())
    {
        setError(error, "The selected preset could not be found.");
        return false;
    }

    const auto parsed = readJson(file);
    auto* object = parsed.getDynamicObject();
    if (object == nullptr
        || static_cast<int>(object->getProperty("schemaVersion")) != presetSchemaVersion)
    {
        setError(error, "The preset file is invalid or uses an unsupported version.");
        return false;
    }

    const auto has = [object](const char* key)
    {
        return object->hasProperty(juce::Identifier(key));
    };
    const char* required[] = {
        "inputGain", "pitchSensitivity", "pitchSmoothing", "pitchBendEnabled", "pitchBendRange", "midiChannel", "midiOutputName"
    };
    for (const auto* key : required)
    {
        if (!has(key))
        {
            setError(error, "The preset is missing required settings.");
            return false;
        }
    }

    PresetData loaded;
    loaded.inputGain = static_cast<double>(object->getProperty("inputGain"));
    loaded.pitchSensitivity = static_cast<double>(object->getProperty("pitchSensitivity"));
    loaded.pitchSmoothing = static_cast<double>(object->getProperty("pitchSmoothing"));
    loaded.pitchBendEnabled = static_cast<bool>(object->getProperty("pitchBendEnabled"));
    if (has("noteTransitionSmoothingEnabled"))
        loaded.noteTransitionSmoothingEnabled = static_cast<bool>(object->getProperty("noteTransitionSmoothingEnabled"));
    if (has("noteTransitionStability")) loaded.noteTransitionStability=static_cast<double>(object->getProperty("noteTransitionStability"));
    if(has("autoTuneEnabled"))loaded.autoTuneEnabled=static_cast<bool>(object->getProperty("autoTuneEnabled"));
    if(has("autoTuneTonic"))loaded.autoTuneTonic=static_cast<int>(object->getProperty("autoTuneTonic"));
    if(has("autoTuneScale"))loaded.autoTuneScale=static_cast<int>(object->getProperty("autoTuneScale"));
    if (has("noiseReductionEnabled")) loaded.noiseReductionEnabled = static_cast<bool>(object->getProperty("noiseReductionEnabled"));
    if (has("noiseReductionStrength")) loaded.noiseReductionStrength = static_cast<double>(object->getProperty("noiseReductionStrength"));
    if (has("pitchStabilityEnabled")) loaded.pitchStabilityEnabled = static_cast<bool>(object->getProperty("pitchStabilityEnabled"));
    if (has("pitchDropToleranceCents")) loaded.pitchDropToleranceCents = static_cast<double>(object->getProperty("pitchDropToleranceCents"));
    if (has("pitchDropHoldFrames")) loaded.pitchDropHoldFrames = static_cast<int>(object->getProperty("pitchDropHoldFrames"));
    if (has("inputChannelMode")) loaded.inputChannelMode = static_cast<int>(object->getProperty("inputChannelMode"));
    loaded.pitchBendRange = static_cast<int>(object->getProperty("pitchBendRange"));
    loaded.midiChannel = static_cast<int>(object->getProperty("midiChannel"));
    loaded.midiOutputName = object->getProperty("midiOutputName").toString().toStdString();
    loaded.sanitise();
    data = std::move(loaded);
    return true;
}

bool PresetManager::deletePreset(const juce::String& name, juce::String* error) const
{
    if (sanitiseName(name).isEmpty())
    {
        setError(error, "The selected preset could not be found.");
        return false;
    }
    const auto file = fileForName(name);
    if (!file.existsAsFile())
    {
        setError(error, "The selected preset could not be found.");
        return false;
    }
    if (!file.deleteFile())
    {
        setError(error, "Could not delete the preset file.");
        return false;
    }
    return true;
}
