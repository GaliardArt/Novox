#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <algorithm>
#include <cmath>
class NoiseReducer final{
public:
 void prepare(double rate,int maxBlock){sampleRate=rate>0?rate:44100;target=juce::jmax(1,(int)(sampleRate*2));juce::dsp::ProcessSpec s;s.sampleRate=sampleRate;s.maximumBlockSize=(juce::uint32)juce::jmax(1,maxBlock);s.numChannels=1;gate.prepare(s);gate.setAttack(2);gate.setRelease(110);gate.setRatio(8);gate.setThreshold(-49.5f);gate.reset();appliedStrength=-1;appliedThreshold=999;wasEnabled=false;request.store(false);calibrating=false;if(state.load()==1)state.store(0);}
 void setEnabled(bool v)noexcept{enabled.store(v,std::memory_order_relaxed);}void setStrength(float v)noexcept{strength.store(std::clamp(v,0.0f,1.0f),std::memory_order_relaxed);}
 void beginCalibration()noexcept{state.store(1,std::memory_order_release);request.store(true,std::memory_order_release);}int getCalibrationState()const noexcept{return state.load(std::memory_order_acquire);}
 float getCalibratedNoiseFloorDb()const noexcept{return floorDb.load(std::memory_order_acquire);}void setCalibratedNoiseFloorDb(float v)noexcept{if(std::isfinite(v)&&v>=-120&&v<=0)floorDb.store(v,std::memory_order_release);}
 void process(float*x,int n)noexcept{if(!x||n<=0)return;if(request.exchange(false)){calibrating=true;energy=0;samples=0;}
  if(calibrating){int m=juce::jmin(n,target-samples);for(int i=0;i<m;++i){double v=x[i];energy+=v*v;}samples+=m;if(samples>=target){double rms=std::sqrt(energy/(double)juce::jmax(1,samples));float db=(float)(20*std::log10(juce::jmax(1.0e-7,rms)));floorDb.store(juce::jlimit(-120.0f,0.0f,db),std::memory_order_release);calibrating=false;state.store(2,std::memory_order_release);}return;}
  bool active=enabled.load(std::memory_order_relaxed);if(!active){if(wasEnabled)gate.reset();wasEnabled=false;return;}if(!wasEnabled){gate.reset();wasEnabled=true;}
  float amount=strength.load(std::memory_order_relaxed),floor=floorDb.load(std::memory_order_acquire);float threshold=floor>-119?juce::jlimit(-60.0f,-24.0f,floor+6.0f+amount*12.0f):-60.0f+amount*30.0f;
  if(std::abs(amount-appliedStrength)>=.005f||std::abs(threshold-appliedThreshold)>=.05f){gate.setThreshold(threshold);appliedStrength=amount;appliedThreshold=threshold;}for(int i=0;i<n;++i)x[i]=gate.processSample(0,x[i]);
 }
private:juce::dsp::NoiseGate<float>gate;std::atomic<bool>enabled{false},request{false};std::atomic<float>strength{.35f},floorDb{-120};std::atomic<int>state{0};double sampleRate=44100,energy=0;int target=88200,samples=0;bool calibrating=false,wasEnabled=false;float appliedStrength=-1,appliedThreshold=999;
};