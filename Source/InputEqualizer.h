#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
class InputEqualizer final
{
public:
 void prepare(double rate) noexcept { sampleRate=rate>0?rate:44100.0;low.reset();mid.reset();high.reset();appliedLow=appliedMid=appliedHigh=999.0f;update(); }
 void setEnabled(bool v) noexcept { enabled.store(v,std::memory_order_relaxed); }
 void setGains(float l,float m,float h) noexcept { lowDb.store(juce::jlimit(-12.0f,12.0f,l));midDb.store(juce::jlimit(-12.0f,12.0f,m));highDb.store(juce::jlimit(-12.0f,12.0f,h)); }
 void process(float*x,int n) noexcept { if(!x||n<=0||!enabled.load(std::memory_order_relaxed))return;update();for(int i=0;i<n;++i){float v=low.processSample(x[i]);v=mid.processSample(v);x[i]=high.processSample(v);} }
private:
 void update() noexcept {const float l=lowDb.load(),m=midDb.load(),h=highDb.load();if(std::abs(l-appliedLow)<.01f&&std::abs(m-appliedMid)<.01f&&std::abs(h-appliedHigh)<.01f)return;
 low.coefficients=juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate,120.0f,.707f,juce::Decibels::decibelsToGain(l));
 mid.coefficients=juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,1000.0f,.8f,juce::Decibels::decibelsToGain(m));
 high.coefficients=juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate,5000.0f,.707f,juce::Decibels::decibelsToGain(h));
 appliedLow=l;appliedMid=m;appliedHigh=h;}
 juce::dsp::IIR::Filter<float> low,mid,high;std::atomic<bool> enabled{false};std::atomic<float> lowDb{0},midDb{0},highDb{0};double sampleRate=44100;float appliedLow=999,appliedMid=999,appliedHigh=999;
};
