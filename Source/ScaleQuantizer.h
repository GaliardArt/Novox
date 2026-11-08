#pragma once
#include <algorithm>
#include <array>
#include <cstdlib>
enum class MusicalScale { chromatic=0, major=1, minor=2 };
class ScaleQuantizer {
public:
 static int quantize(int note,int tonic,MusicalScale scale) noexcept {
  if(note<0||note>127||scale==MusicalScale::chromatic)return note;
  static constexpr std::array<int,7> maj{0,2,4,5,7,9,11},min{0,2,3,5,7,8,10};
  const auto& steps=scale==MusicalScale::minor?min:maj; tonic=((tonic%12)+12)%12;
  int best=note,dist=128;
  for(int n=std::max(0,note-6);n<=std::min(127,note+6);++n) {
   int pc=((n-tonic)%12+12)%12;
   if(std::find(steps.begin(),steps.end(),pc)==steps.end())continue;
   int d=std::abs(n-note);if(d<dist||(d==dist&&n<best)){best=n;dist=d;}
  } return best;
 }
};