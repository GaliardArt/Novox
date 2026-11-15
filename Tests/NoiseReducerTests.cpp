#include "NoiseReducer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

int main()
{
    int failures = 0;
    const auto expect = [&failures](bool ok, const char* message)
    {
        if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    };

    NoiseReducer reducer;
    reducer.prepare(48000.0, 512);
    expect(reducer.isAvailable(), "RNNoise initializes at its native 48 kHz rate");

    std::array<float, 128> bypass {};
    for (size_t i = 0; i < bypass.size(); ++i)
        bypass[i] = 0.1f * std::sin(static_cast<float>(i) * 0.13f);
    const auto original = bypass;
    reducer.process(bypass.data(), static_cast<int>(bypass.size()));
    expect(bypass == original, "disabled processing is a bit-transparent bypass");

    reducer.setEnabled(true);
    std::array<float, 4800> signal {};
    for (size_t i = 0; i < signal.size(); ++i)
        signal[i] = 0.08f * std::sin(2.0f * 3.14159265358979323846f * 220.0f *
                                   static_cast<float>(i) / 48000.0f)
                  + 0.006f * std::sin(static_cast<float>(i) * 1.731f);
    // Deliberately irregular blocks exercise persistent 480-sample framing.
    int offset = 0;
    constexpr int blocks[] { 73, 191, 512, 47, 333, 701, 128, 1024, 1791 };
    for (const int block : blocks)
    {
        if (offset >= static_cast<int>(signal.size())) break;
        const int count = std::min(block, static_cast<int>(signal.size()) - offset);
        reducer.process(signal.data() + offset, count);
        offset += count;
    }
    if (offset < static_cast<int>(signal.size()))
        reducer.process(signal.data() + offset, static_cast<int>(signal.size()) - offset);

    const bool finite = std::all_of(signal.begin(), signal.end(),
                                    [](float x) { return std::isfinite(x); });
    expect(finite, "RNNoise output remains finite across partial and multiple frames");

    reducer.setEnabled(false);
    std::array<float, 64> afterDisable {};
    for (size_t i = 0; i < afterDisable.size(); ++i)
        afterDisable[i] = 0.05f * std::cos(static_cast<float>(i) * 0.2f);
    const auto unchanged = afterDisable;
    reducer.process(afterDisable.data(), static_cast<int>(afterDisable.size()));
    expect(afterDisable == unchanged, "disabling RNNoise restores transparent bypass");

    NoiseReducer unsupportedRate;
    unsupportedRate.prepare(44100.0, 512);
    expect(!unsupportedRate.isAvailable(), "unsupported sample rate safely reports bypass rather than misprocessing at 48 kHz");

    if (failures != 0) std::cerr << failures << " RNNoise test(s) failed.\n";
    return failures == 0 ? 0 : 1;
}
