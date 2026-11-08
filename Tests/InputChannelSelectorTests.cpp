#include "InputChannelSelector.h"
#include <iostream>

int main()
{
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message)
    {
        if (!condition)
        {
            ++failures;
            std::cerr << "[FAIL] " << message << '\n';
        }
        else
        {
            std::cout << "[PASS] " << message << '\n';
        }
    };

    float leftQuiet[4] { 0.05f, -0.05f, 0.05f, -0.05f };
    float rightLoud[4] { 0.30f, -0.30f, 0.30f, -0.30f };
    const float* channels[] { leftQuiet, rightLoud };

    InputChannelSelector selector;
    check(selector.process(InputChannelMode::automatic, channels, 2, 4, 0.01f) == 1,
          "automatic mode initially selects the strongest channel");

    float leftLoud[4] { 0.40f, -0.40f, 0.40f, -0.40f };
    const float* swapped[] { leftLoud, rightLoud };
    for (int i = 0; i < 7; ++i)
        check(selector.process(InputChannelMode::automatic, swapped, 2, 4, 0.01f) == 1,
              "brief level change does not immediately switch channels");
    check(selector.process(InputChannelMode::automatic, swapped, 2, 4, 0.01f) == 0,
          "stronger channel is selected after 80 ms confirmation");

    selector.reset();
    check(selector.process(InputChannelMode::automatic, channels, 2, 4, 0.01f) == 1,
          "automatic selection initializes after reset");
    float leftSimilar[4] { 0.34f, -0.34f, 0.34f, -0.34f };
    const float* similar[] { leftSimilar, rightLoud };
    for (int i = 0; i < 20; ++i)
        check(selector.process(InputChannelMode::automatic, similar, 2, 4, 0.01f) == 1,
              "similar channel levels do not cause selection flapping");

    check(selector.process(InputChannelMode::left, channels, 2, 4, 0.01f) == 0,
          "left mode switches immediately");
    check(selector.process(InputChannelMode::right, channels, 2, 4, 0.01f) == 1,
          "right mode switches immediately");
    check(selector.process(InputChannelMode::stereoMix, channels, 2, 4, 0.01f) == -1,
          "stereo mix leaves channel selection open");
    const float* mono[] { leftQuiet };
    check(selector.process(InputChannelMode::right, mono, 1, 4, 0.01f) == 0,
          "right mode falls back safely for mono input");
    check(selector.process(InputChannelMode::automatic, nullptr, 0, 4, 0.01f) == -1,
          "missing input is handled safely");

    std::cout << "Failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
