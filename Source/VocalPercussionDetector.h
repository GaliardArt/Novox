#pragma once
#include <array>
#include <cstdint>
enum class VocalPercussionType : uint8_t { none = 0, kick, snare, hiHat };
struct VocalPercussionResult { VocalPercussionType type = VocalPercussionType::none; float confidence = 0.0f; float rms = 0.0f; uint64_t sequence = 0; };
class VocalPercussionDetector {
public:
    void prepare(double sampleRate) noexcept;
    VocalPercussionResult process(const float* samples, int numSamples) noexcept;
    void reset() noexcept;
private:
    static constexpr int analysisSize = 256;
    void analyseFrame() noexcept;
    std::array<float, analysisSize> frame {};
    int framePosition = 0;
    double currentSampleRate = 48000.0;
    float smoothedRms = 0.0f;
    float noiseFloor = 0.0015f;
    int samplesSinceHit = 0;
    uint64_t sequence = 0;
    VocalPercussionResult latest;
};
