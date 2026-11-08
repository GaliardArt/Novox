#include "VocalPercussionDetector.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr float minimumRms = 0.006f;
constexpr float minimumOnset = 1.42f;
float onePoleCoefficient(double cutoff, double sampleRate) noexcept {
    return static_cast<float>(1.0 - std::exp(-2.0 * pi * cutoff / sampleRate));
}
}
void VocalPercussionDetector::prepare(double sampleRate) noexcept {
    currentSampleRate = sampleRate > 1000.0 ? sampleRate : 48000.0;
    reset();
}
void VocalPercussionDetector::reset() noexcept {
    frame.fill(0.0f); framePosition = 0; smoothedRms = 0.0f; noiseFloor = 0.0015f;
    samplesSinceHit = static_cast<int>(currentSampleRate * 0.05); sequence = 0; latest = {};
}
VocalPercussionResult VocalPercussionDetector::process(const float* samples, int numSamples) noexcept {
    latest.type = VocalPercussionType::none; latest.confidence = 0.0f;
    if (samples == nullptr || numSamples <= 0) return latest;
    for (int i = 0; i < numSamples; ++i) {
        const float sample = std::isfinite(samples[i]) ? std::clamp(samples[i], -4.0f, 4.0f) : 0.0f;
        frame[static_cast<size_t>(framePosition++)] = sample;
        if (framePosition == analysisSize) { analyseFrame(); framePosition = 0; }
        if (samplesSinceHit < static_cast<int>(currentSampleRate)) ++samplesSinceHit;
    }
    return latest;
}
void VocalPercussionDetector::analyseFrame() noexcept {
    double totalEnergy=0.0, lowEnergy=0.0, midEnergy=0.0, highEnergy=0.0, attackEnergy=0.0, tailEnergy=0.0;
    int zeroCrossings=0; float peak=0.0f, previous=frame[0], lowState=previous, lowMidState=previous;
    const float lowCoefficient=onePoleCoefficient(160.0,currentSampleRate);
    const float midCoefficient=onePoleCoefficient(4200.0,currentSampleRate);
    for (int i=0;i<analysisSize;++i) {
        const float sample=frame[static_cast<size_t>(i)];
        lowState += lowCoefficient*(sample-lowState);
        lowMidState += midCoefficient*(sample-lowMidState);
        const float low=lowState, mid=lowMidState-lowState, high=sample-lowMidState;
        const double e=static_cast<double>(sample)*sample;
        totalEnergy+=e; lowEnergy+=static_cast<double>(low)*low;
        midEnergy+=static_cast<double>(mid)*mid; highEnergy+=static_cast<double>(high)*high;
        if(i<analysisSize/4) attackEnergy+=e; else tailEnergy+=e;
        peak=std::max(peak,std::abs(sample));
        if((sample>=0.0f&&previous<0.0f)||(sample<0.0f&&previous>=0.0f)) ++zeroCrossings;
        previous=sample;
    }
    const float rms=static_cast<float>(std::sqrt(totalEnergy/analysisSize));
    const float lowRatio=totalEnergy>1.0e-12?static_cast<float>(lowEnergy/totalEnergy):0.0f;
    const float midRatio=totalEnergy>1.0e-12?static_cast<float>(midEnergy/totalEnergy):0.0f;
    const float highRatio=totalEnergy>1.0e-12?static_cast<float>(highEnergy/totalEnergy):0.0f;
    const float zcr=static_cast<float>(zeroCrossings)/analysisSize;
    const float crest=rms>1.0e-6f?peak/rms:0.0f;
    const float attackTail=static_cast<float>(std::sqrt(attackEnergy)/std::max(std::sqrt(tailEnergy),1.0e-5));
    const float onset=rms/std::max({smoothedRms,noiseFloor*2.5f,0.0025f});
    if(rms<std::max(0.025f,noiseFloor*3.0f))
        noiseFloor=std::clamp(noiseFloor*0.995f+rms*0.005f,0.0005f,0.03f);
    smoothedRms=smoothedRms*0.78f+rms*0.22f;
    if(rms<std::max(minimumRms,noiseFloor*4.0f)||onset<minimumOnset||crest<1.35f||attackTail<0.72f) return;
    VocalPercussionType detected=VocalPercussionType::none; float confidence=0.0f;
    if(lowRatio>=0.52f&&zcr<=0.14f&&midRatio<0.42f) {
        detected=VocalPercussionType::kick;
        confidence=0.58f+(lowRatio-0.52f)*0.75f+std::clamp((0.14f-zcr)*1.2f,0.0f,0.12f);
    } else if(highRatio>=0.42f&&zcr>=0.20f&&midRatio<0.38f&&attackTail>=0.78f) {
        detected=VocalPercussionType::hiHat;
        confidence=0.54f+(highRatio-0.42f)*0.55f+std::clamp((zcr-0.20f)*0.65f,0.0f,0.12f);
    } else if(midRatio>=0.28f&&zcr>=0.12f&&lowRatio<0.55f) {
        detected=VocalPercussionType::snare;
        confidence=0.53f+(midRatio-0.28f)*0.65f+std::clamp((zcr-0.12f)*0.45f,0.0f,0.12f);
    }
    if(detected==VocalPercussionType::none) return;
    const int refractoryMs=detected==VocalPercussionType::hiHat?28:42;
    if(samplesSinceHit<static_cast<int>(currentSampleRate*refractoryMs/1000.0)) return;
    ++sequence; latest.type=detected; latest.confidence=std::clamp(confidence,0.35f,0.98f);
    latest.rms=rms; latest.sequence=sequence; samplesSinceHit=0;
}
