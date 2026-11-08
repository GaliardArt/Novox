#pragma once
#include "PitchDetector.h"
#include <algorithm>
#include <cmath>
class PitchStabilityGuard
{
public:
 void configure(bool on,float cents,int frames) noexcept {const int f=std::clamp(frames,1,8);const float c=std::clamp(cents,10.0f,150.0f);const bool changed=enabled!=on||holdFrames!=f||std::abs(dropToleranceCents-c)>.01f;enabled=on;holdFrames=f;dropToleranceCents=c;if(changed)reset();}
 PitchResult process(const PitchResult& in) noexcept {
  if(!enabled){reset();return in;}
  if(!in.voiced||!(in.frequencyHz>0.0f)){if(stable.voiced&&pending<holdFrames){++pending;return held(in);}stable={};pending=0;return in;}
  if(!stable.voiced||!(stable.frequencyHz>0.0f)){stable=in;pending=0;return in;}
  const float cents=1200.0f*std::log2(in.frequencyHz/stable.frequencyHz);
  if(cents < -dropToleranceCents){++pending;if(pending<holdFrames)return held(in);stable=in;pending=0;return in;}
  pending=0;stable=in;return in;
 }
 void reset() noexcept {stable={};pending=0;}
private:
 PitchResult held(const PitchResult& in)const noexcept{auto out=stable;out.sequence=in.sequence;out.rms=in.rms;return out;}
 PitchResult stable;int pending=0,holdFrames=3;float dropToleranceCents=45.0f;bool enabled=false;
};
