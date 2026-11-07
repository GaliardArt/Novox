#include "PitchDetector.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
    constexpr double pi = 3.14159265358979323846;
    int failures = 0;
    int checks = 0;

    void check(bool condition, const std::string& name, const std::string& detail = {})
    {
        ++checks;
        if (condition)
            std::cout << "[PASS] " << name << (detail.empty() ? "" : " — " + detail) << '\n';
        else
        {
            ++failures;
            std::cerr << "[FAIL] " << name << (detail.empty() ? "" : " — " + detail) << '\n';
        }
    }

    PitchResult analyseSignal(const std::vector<float>& signal, double sampleRate, int blockSize = 256)
    {
        PitchDetector detector;
        detector.prepare(sampleRate, blockSize);
        PitchResult latest;
        for (size_t offset = 0; offset < signal.size(); offset += static_cast<size_t>(blockSize))
        {
            const auto count = std::min(static_cast<size_t>(blockSize), signal.size() - offset);
            latest = detector.process(signal.data() + offset, static_cast<int>(count));
        }
        return latest;
    }

    std::vector<float> makeSine(double frequency, double sampleRate, double seconds, float amplitude = 0.5f)
    {
        const auto count = static_cast<size_t>(sampleRate * seconds);
        std::vector<float> signal(count);
        for (size_t i = 0; i < count; ++i)
            signal[i] = amplitude * static_cast<float>(std::sin(2.0 * pi * frequency * i / sampleRate));
        return signal;
    }

    void testKnownFrequencies()
    {
        struct Case { double hz; const char* label; };
        const Case cases[] = {
            { 82.4069, "E2 / low vocal range" },
            { 110.0, "A2" },
            { 220.0, "A3" },
            { 440.0, "A4 / tuning reference" },
            { 880.0, "A5 / upper range" }
        };

        for (const auto& item : cases)
        {
            const auto result = analyseSignal(makeSine(item.hz, 48000.0, 0.35), 48000.0);
            const double errorPercent = result.frequencyHz > 0.0f
                ? std::abs(result.frequencyHz - item.hz) / item.hz * 100.0 : 1000.0;
            check(result.voiced && errorPercent <= 2.0,
                  std::string("Known frequency: ") + item.label,
                  "expected " + std::to_string(item.hz) + " Hz, got "
                    + std::to_string(result.frequencyHz) + " Hz (" 
                    + std::to_string(errorPercent) + "% error)");
        }
    }

    void testSampleRates()
    {
        for (const double sampleRate : { 44100.0, 48000.0 })
        {
            const auto result = analyseSignal(makeSine(196.0, sampleRate, 0.35), sampleRate);
            const auto error = std::abs(result.frequencyHz - 196.0f);
            check(result.voiced && error <= 2.0f,
                  "Sample-rate handling at " + std::to_string(static_cast<int>(sampleRate)) + " Hz",
                  "detected " + std::to_string(result.frequencyHz) + " Hz");
        }
    }

    void testSilenceAndLowLevel()
    {
        std::vector<float> silence(48000, 0.0f);
        auto result = analyseSignal(silence, 48000.0);
        check(!result.voiced && result.frequencyHz == 0.0f, "Silence is not classified as a note");

        result = analyseSignal(makeSine(440.0, 48000.0, 0.35, 0.002f), 48000.0);
        check(!result.voiced, "Very low-level signal is rejected by the silence gate");
    }

    void testHarmonicSignal()
    {
        constexpr double sampleRate = 48000.0;
        constexpr double fundamental = 220.0;
        auto signal = makeSine(fundamental, sampleRate, 0.35, 0.35f);
        for (size_t i = 0; i < signal.size(); ++i)
            signal[i] += 0.15f * static_cast<float>(std::sin(2.0 * pi * 2.0 * fundamental * i / sampleRate));

        const auto result = analyseSignal(signal, sampleRate);
        check(result.voiced && std::abs(result.frequencyHz - fundamental) <= 3.0f,
              "Fundamental survives a strong second harmonic",
              "detected " + std::to_string(result.frequencyHz) + " Hz");
    }

    void testNoiseRejection()
    {
        std::mt19937 generator(0x4e4f564f);
        std::uniform_real_distribution<float> distribution(-0.25f, 0.25f);
        std::vector<float> noise(48000);
        std::generate(noise.begin(), noise.end(), [&] { return distribution(generator); });
        const auto result = analyseSignal(noise, 48000.0);
        check(!result.voiced, "Broadband noise is not classified as a stable pitch",
              "confidence " + std::to_string(result.confidence));
    }

    void testInvalidInput()
    {
        PitchDetector detector;
        detector.prepare(48000.0, 256);
        const auto nullResult = detector.process(nullptr, 256);
        const float sample = 0.1f;
        const auto emptyResult = detector.process(&sample, 0);
        check(!nullResult.voiced && !emptyResult.voiced, "Null and empty input are handled safely");
    }
}

int main()
{
    std::cout << "NOVOX PitchDetector regression tests\n";
    testKnownFrequencies();
    testSampleRates();
    testSilenceAndLowLevel();
    testHarmonicSignal();
    testNoiseRejection();
    testInvalidInput();

    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
