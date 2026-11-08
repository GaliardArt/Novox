#include "PitchSmoother.h"
#include <cmath>
#include <iostream>
#include <string>

namespace
{
    int checks = 0;
    int failures = 0;
    void check(bool condition, const std::string& name)
    {
        ++checks;
        if (condition) std::cout << "[PASS] " << name << '\n';
        else { ++failures; std::cerr << "[FAIL] " << name << '\n'; }
    }

    void testTransparentMode()
    {
        PitchSmoother smoother;
        check(std::abs(smoother.process(0.35f, 0.0f) - 0.35f) < 0.0001f,
              "Zero smoothing passes pitch through without latency");
        check(std::abs(smoother.process(-0.2f, 0.0f) + 0.2f) < 0.0001f,
              "Transparent mode follows expressive pitch changes immediately");
    }

    void testSmoothingAndConvergence()
    {
        PitchSmoother smoother;
        smoother.process(0.0f, 50.0f);
        const float first = smoother.process(0.8f, 50.0f);
        check(first > 0.0f && first < 0.8f,
              "Smoothing attenuates an abrupt pitch jump");
        float latest = first;
        for (int i = 0; i < 80; ++i)
            latest = smoother.process(0.8f, 50.0f);
        check(std::abs(latest - 0.8f) < 0.002f,
              "Smoothed pitch converges to a sustained target");
    }

    void testMicroVariationDeadband()
    {
        PitchSmoother smoother;
        smoother.process(0.2f, 100.0f);
        const float baseline = smoother.process(0.2f, 100.0f);
        const float jitter = smoother.process(0.22f, 100.0f);
        check(std::abs(jitter - baseline) < 0.0001f,
              "High smoothing rejects small natural pitch fluctuations");
        const float expressive = smoother.process(0.5f, 100.0f);
        check(expressive > jitter,
              "Larger intentional pitch movement remains responsive");
    }

    void testResetAndInvalid()
    {
        PitchSmoother smoother;
        smoother.process(0.7f, 80.0f);
        smoother.reset();
        check(!smoother.isInitialized(), "Reset clears smoothing history");
        check(std::abs(smoother.process(0.1f, 80.0f) - 0.1f) < 0.0001f,
              "First pitch after reset is not delayed");
        const float before = smoother.getCurrentValue();
        const float after = smoother.process(std::nanf(""), 80.0f);
        check(std::abs(before - after) < 0.0001f,
              "Non-finite pitch input does not corrupt filter state");
    }

    void testParameterClamping()
    {
        PitchSmoother low, high;
        low.process(0.0f, -100.0f);
        high.process(0.0f, 1000.0f);
        check(std::abs(low.process(0.5f, -100.0f) - 0.5f) < 0.0001f,
              "Smoothing parameter clamps below zero");
        check(high.process(1.0f, 1000.0f) > 0.0f
              && high.process(1.0f, 1000.0f) < 1.0f,
              "Smoothing parameter clamps above one hundred");
    }
}

int main()
{
    std::cout << "NOVOX PitchSmoother regression tests\n";
    testTransparentMode();
    testSmoothingAndConvergence();
    testMicroVariationDeadband();
    testResetAndInvalid();
    testParameterClamping();
    std::cout << "\nChecks: " << checks << ", failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
