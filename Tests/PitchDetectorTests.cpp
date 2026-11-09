#include "PitchDetector.h"

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

    PitchResult analyseSignal(const std::vector<float>& signal, double sampleRate, int blockSize = 256,
                              float sensitivity = 0.5f)
    {
        PitchDetector detector;
        detector.prepare(sampleRate, blockSize);
        detector.setSensitivity(sensitivity);
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
        for (const double sampleRate : { 44100.0, 48000.0, 96000.0 })
        {
            const auto result = analyseSignal(makeSine(196.0, sampleRate, 0.35), sampleRate);
            const auto error = std::abs(result.frequencyHz - 196.0f);
            check(result.voiced && error <= 2.0f,
                  "Sample-rate handling at " + std::to_string(static_cast<int>(sampleRate)) + " Hz",
                  "detected " + std::to_string(result.frequencyHz) + " Hz");
        }
    }

    void testHighSampleRateBassRange()
    {
        const auto result = analyseSignal(makeSine(82.4069, 96000.0, 0.45), 96000.0);
        check(result.voiced && std::abs(result.frequencyHz - 82.4069f) < 3.0f,
              "Low E2 remains detectable at 96 kHz input rate",
              "detected " + std::to_string(result.frequencyHz) + " Hz");
    }

    void testSilenceAndLowLevel()
    {
        std::vector<float> silence(48000, 0.0f);
        auto result = analyseSignal(silence, 48000.0);
        check(!result.voiced && result.frequencyHz == 0.0f, "Silence is not classified as a note");

        result = analyseSignal(makeSine(440.0, 48000.0, 0.35, 0.002f), 48000.0);
        check(!result.voiced, "Very low-level signal is rejected by default sensitivity");
    }

    void testSensitivityControl()
    {
        const auto quietSignal = makeSine(440.0, 48000.0, 0.35, 0.004f);
        const auto lowSensitivity = analyseSignal(quietSignal, 48000.0, 256, 0.0f);
        const auto highSensitivity = analyseSignal(quietSignal, 48000.0, 256, 1.0f);
        check(!lowSensitivity.voiced,
              "Low sensitivity rejects a quiet signal");
        check(highSensitivity.voiced && std::abs(highSensitivity.frequencyHz - 440.0f) < 3.0f,
              "High sensitivity tracks a quiet but pitched signal");
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


    void testDominantThirdHarmonic()
    {
        constexpr double sampleRate = 48000.0;
        constexpr double fundamental = 220.0;
        auto signal = makeSine(fundamental, sampleRate, 0.4, 0.025f);
        const auto second = makeSine(fundamental * 2.0, sampleRate, 0.4, 0.10f);
        const auto third = makeSine(fundamental * 3.0, sampleRate, 0.4, 0.45f);
        for (size_t i = 0; i < signal.size(); ++i)
            signal[i] += second[i] + third[i];

        const auto result = analyseSignal(signal, sampleRate);
        check(result.voiced && std::abs(result.frequencyHz - fundamental) <= 5.0f,
              "Fundamental is retained when the third harmonic dominates",
              "expected 220 Hz, detected " + std::to_string(result.frequencyHz) + " Hz");

        const auto pureTone = analyseSignal(makeSine(660.0, sampleRate, 0.4), sampleRate);
        check(pureTone.voiced && std::abs(pureTone.frequencyHz - 660.0f) <= 5.0f,
              "Third-subharmonic check does not lower a clean high tone",
              "detected " + std::to_string(pureTone.frequencyHz) + " Hz");
    }

    void testAntiAliasing()
    {
        constexpr double sampleRate = 48000.0;
        constexpr double fundamental = 220.0;
        auto mixed = makeSine(fundamental, sampleRate, 0.35, 0.35f);
        const auto highTone = makeSine(15000.0, sampleRate, 0.35, 0.45f);
        for (size_t i = 0; i < mixed.size(); ++i)
            mixed[i] += highTone[i];

        const auto mixedResult = analyseSignal(mixed, sampleRate);
        check(mixedResult.voiced && std::abs(mixedResult.frequencyHz - fundamental) <= 4.0f,
              "Anti-alias filter preserves the voice fundamental beside high-frequency content",
              "detected " + std::to_string(mixedResult.frequencyHz) + " Hz");

        const auto highOnly = analyseSignal(highTone, sampleRate);
        check(!highOnly.voiced,
              "Out-of-band high-frequency tone is rejected after decimation",
              "detected " + std::to_string(highOnly.frequencyHz) + " Hz");
    }

    void testOctaveErrorCorrection()
    {
        constexpr double sampleRate = 48000.0;
        constexpr double fundamental = 220.0;
        auto signal = makeSine(fundamental, sampleRate, 0.35, 0.03f);
        for (size_t i = 0; i < signal.size(); ++i)
            signal[i] += 0.45f * static_cast<float>(std::sin(2.0 * pi * 2.0 * fundamental * i / sampleRate));

        const auto result = analyseSignal(signal, sampleRate);
        check(result.voiced && std::abs(result.frequencyHz - fundamental) <= 4.0f,
              "Octave correction detects a weaker fundamental beneath a dominant second harmonic",
              "expected 220 Hz, detected " + std::to_string(result.frequencyHz) + " Hz");

        const auto pureTone = analyseSignal(makeSine(440.0, sampleRate, 0.35), sampleRate);
        check(pureTone.voiced && std::abs(pureTone.frequencyHz - 440.0f) <= 4.0f,
              "Octave correction does not lower a clean tone by one octave",
              "detected " + std::to_string(pureTone.frequencyHz) + " Hz");
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

    void testAdaptiveAnalysisWindow()
    {
        const auto bass = analyseSignal(makeSine(110.0, 48000.0, 0.45), 48000.0);
        const auto high = analyseSignal(makeSine(440.0, 48000.0, 0.45), 48000.0);
        check(bass.voiced && bass.analysisWindowSamples == 2048, "Bass notes use the longer analysis window", "window " + std::to_string(bass.analysisWindowSamples));
        check(high.voiced && high.analysisWindowSamples == 1024, "Higher notes use the shorter low-latency window", "window " + std::to_string(high.analysisWindowSamples));
    }

    void testLowLatencyProgressiveWindows()
    {
        PitchDetector highDetector;
        highDetector.prepare(48000.0, 64);
        highDetector.setLowLatencyMode(true);
        const auto high = makeSine(440.0, 48000.0, 0.08);
        PitchResult highResult;
        for (size_t offset = 0; offset < high.size(); offset += 64)
            highResult = highDetector.process(high.data() + offset, static_cast<int>(std::min<size_t>(64, high.size() - offset)));
        check(highResult.voiced && std::abs(highResult.frequencyHz - 440.0f) < 5.0f && highResult.analysisWindowSamples == 128,
              "Low-latency mode uses a 128-sample window for high notes",
              "detected " + std::to_string(highResult.frequencyHz) + " Hz / window " + std::to_string(highResult.analysisWindowSamples));

        PitchDetector bassDetector;
        bassDetector.prepare(48000.0, 64);
        bassDetector.setLowLatencyMode(true);
        const auto bass = makeSine(82.4069, 48000.0, 0.30);
        PitchResult bassResult;
        for (size_t offset = 0; offset < bass.size(); offset += 64)
            bassResult = bassDetector.process(bass.data() + offset, static_cast<int>(std::min<size_t>(64, bass.size() - offset)));
        check(bassResult.voiced && std::abs(bassResult.frequencyHz - 82.4069f) < 4.0f && bassResult.analysisWindowSamples >= 1024,
              "Low-latency mode expands its window to retain bass accuracy",
              "detected " + std::to_string(bassResult.frequencyHz) + " Hz / window " + std::to_string(bassResult.analysisWindowSamples));
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
    testHighSampleRateBassRange();
    testSilenceAndLowLevel();
    testSensitivityControl();
    testHarmonicSignal();
    testAntiAliasing();
    testOctaveErrorCorrection();
    testDominantThirdHarmonic();
    testNoiseRejection();
    testAdaptiveAnalysisWindow();
    testLowLatencyProgressiveWindows();
    testInvalidInput();

    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
