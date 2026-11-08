#include "NoiseReducer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
namespace
{
    int failures = 0;
    void expect(bool condition, const char* message)
    {
        if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    }
    float rms(const float* samples, int count)
    {
        double energy = 0.0;
        for (int i = 0; i < count; ++i) energy += static_cast<double>(samples[i]) * samples[i];
        return static_cast<float>(std::sqrt(energy / static_cast<double>(count)));
    }
}
int main()
{
    NoiseReducer reducer;
    reducer.prepare(48000.0, 512);
    float bypass[64], original[64];
    for (int i = 0; i < 64; ++i) bypass[i] = 0.1f * std::sin(static_cast<float>(i) * 0.17f);
    std::copy(std::begin(bypass), std::end(bypass), std::begin(original));
    reducer.process(bypass, 64);
    expect(std::equal(std::begin(bypass), std::end(bypass), std::begin(original)), "disabled noise reduction is a transparent bypass");
    reducer.setStrength(0.65f);
    reducer.setEnabled(true);
    constexpr int sampleCount = 9600;
    float quietNoise[sampleCount];
    for (int i = 0; i < sampleCount; ++i) quietNoise[i] = 0.001f * std::sin(static_cast<float>(i) * 0.13f);
    reducer.process(quietNoise, sampleCount);
    expect(rms(quietNoise + sampleCount / 2, sampleCount / 2) < 0.0005f, "gate attenuates a signal well below its threshold");
    float voice[512];
    for (int i = 0; i < 512; ++i) voice[i] = 0.2f * std::sin(static_cast<float>(i) * 0.11f);
    reducer.process(voice, 512);
    expect(rms(voice, 512) > 0.08f, "gate opens for a strong vocal-level signal");
    reducer.setEnabled(false);
    float afterDisable[64];
    for (int i = 0; i < 64; ++i) afterDisable[i] = 0.1f * std::sin(static_cast<float>(i) * 0.17f);
    std::copy(std::begin(afterDisable), std::end(afterDisable), std::begin(original));
    reducer.process(afterDisable, 64);
    expect(std::equal(std::begin(afterDisable), std::end(afterDisable), std::begin(original)), "disabling the gate restores transparent signal");
    NoiseReducer calibrated;calibrated.prepare(1000,512);calibrated.beginCalibration();float room[512];for(int b=0;b<4;++b){for(int i=0;i<512;++i)room[i]=.002f*std::sin((float)(b*512+i)*.13f);calibrated.process(room,512);}
    expect(calibrated.getCalibrationState()==2,"room noise calibration completes");expect(calibrated.getCalibratedNoiseFloorDb()<-50&&calibrated.getCalibratedNoiseFloorDb()>-65,"calibration estimates noise floor in dBFS");
    calibrated.setStrength(.5f);calibrated.setEnabled(true);float quiet[4096];for(int i=0;i<4096;++i)quiet[i]=.002f*std::sin((float)i*.13f);calibrated.process(quiet,4096);expect(rms(quiet+2048,2048)<.001f,"calibrated gate attenuates measured room noise");
    if (failures != 0) std::cerr << failures << " noise reducer test(s) failed.\n";
    return failures == 0 ? 0 : 1;
}
