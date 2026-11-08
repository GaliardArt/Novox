#include "VocalPercussionDetector.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
    constexpr double pi = 3.14159265358979323846;
    int failures = 0;
    int checks = 0;

    void check(bool condition, const std::string& name)
    {
        ++checks;
        if (condition)
            std::cout << "[PASS] " << name << '\n';
        else
        {
            ++failures;
            std::cerr << "[FAIL] " << name << '\n';
        }
    }

    VocalPercussionResult process(VocalPercussionDetector& detector, const std::vector<float>& signal)
    {
        VocalPercussionResult result;
        for (size_t offset = 0; offset < signal.size(); offset += 128)
        {
            const auto count = std::min<size_t>(128, signal.size() - offset);
            const auto next = detector.process(signal.data() + offset, static_cast<int>(count));
            if (next.type != VocalPercussionType::none)
                result = next;
        }
        return result;
    }

    std::vector<float> makeKick()
    {
        std::vector<float> signal(4096, 0.0f);
        for (size_t i = 0; i < signal.size(); ++i)
        {
            const float envelope = std::exp(-static_cast<float>(i) / 900.0f);
            signal[i] = 0.65f * envelope
                * static_cast<float>(std::sin(2.0 * pi * 75.0 * i / 48000.0));
        }
        return signal;
    }

    std::vector<float> makeNoiseBurst(float amplitude, int count, unsigned seed)
    {
        std::mt19937 generator(seed);
        std::uniform_real_distribution<float> distribution(-amplitude, amplitude);
        std::vector<float> signal(4096, 0.0f);
        for (int i = 0; i < count; ++i)
        {
            const float envelope = std::exp(-static_cast<float>(i) / (count * 0.32f));
            signal[static_cast<size_t>(i)] = distribution(generator) * envelope;
        }
        return signal;
    }
}

int main()
{
    std::cout << "NOVOX VocalPercussionDetector regression tests\n";

    VocalPercussionDetector detector;
    detector.prepare(48000.0);

    const auto kick = process(detector, makeKick());
    check(kick.type == VocalPercussionType::kick, "Low-frequency transient is classified as kick");

    detector.reset();
    const auto snare = process(detector, makeNoiseBurst(0.75f, 1800, 0x534e4152));
    check(snare.type == VocalPercussionType::snare, "Loud broadband transient is classified as snare");

    detector.reset();
    const auto hat = process(detector, makeNoiseBurst(0.22f, 900, 0x484154));
    check(hat.type == VocalPercussionType::hiHat, "Short, quieter high-frequency transient is classified as hi-hat");

    detector.reset();
    const auto quiet = process(detector, std::vector<float>(8192, 0.0f));
    check(quiet.type == VocalPercussionType::none, "Silence does not trigger percussion");

    detector.reset();
    std::vector<float> sustained(8192);
    for (size_t i = 0; i < sustained.size(); ++i)
        sustained[i] = 0.2f * static_cast<float>(std::sin(2.0 * pi * 220.0 * i / 48000.0));
    const auto voice = process(detector, sustained);
    check(voice.type == VocalPercussionType::none, "Sustained pitched voice does not trigger percussion");

    const auto invalid = detector.process(nullptr, 256);
    check(invalid.type == VocalPercussionType::none, "Null input is handled safely");

    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
