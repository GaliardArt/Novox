#include "VocalPercussionDetector.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>
namespace {
constexpr double pi=3.14159265358979323846; int failures=0;
void check(bool ok,const std::string& name){std::cout<<(ok?"[PASS] ":"[FAIL] ")<<name<<'\n';if(!ok)++failures;}
VocalPercussionResult process(VocalPercussionDetector& d,const std::vector<float>& x){
 VocalPercussionResult result;
 for(size_t i=0;i<x.size();i+=128){auto r=d.process(x.data()+i,static_cast<int>(std::min<size_t>(128,x.size()-i)));if(r.type!=VocalPercussionType::none)result=r;}
 return result;
}
std::vector<float> kick(float gain=0.65f){
 std::vector<float>x(4096);for(size_t i=0;i<x.size();++i)x[i]=gain*std::exp(-static_cast<float>(i)/900.0f)*static_cast<float>(std::sin(2*pi*75*i/48000.0));return x;
}
std::vector<float> noiseBurst(float gain,int count,unsigned seed,bool highPass){
 std::mt19937 gen(seed);std::uniform_real_distribution<float>dist(-gain,gain);std::vector<float>x(4096);float lp=0.0f;
 for(int i=0;i<count;++i){float n=dist(gen);lp+=(highPass?0.48f:0.16f)*(n-lp);float s=highPass?n-lp:lp;x[static_cast<size_t>(i)]=s*std::exp(-static_cast<float>(i)/(count*0.32f));}return x;
}
}
int main(){
 VocalPercussionDetector d;d.prepare(48000.0);
 check(process(d,kick()).type==VocalPercussionType::kick,"low-frequency transient is kick");
 d.reset();check(process(d,noiseBurst(0.75f,1800,0x534e4152,false)).type==VocalPercussionType::snare,"mid-band noisy transient is snare");
 d.reset();check(process(d,noiseBurst(0.35f,900,0x484154,true)).type==VocalPercussionType::hiHat,"short high-frequency transient is hi-hat");
 d.reset();check(process(d,std::vector<float>(8192,0.0f)).type==VocalPercussionType::none,"silence is rejected");
 d.reset();std::vector<float>hum(8192);for(size_t i=0;i<hum.size();++i)hum[i]=0.2f*std::sin(2*pi*220*i/48000.0);
 check(process(d,hum).type==VocalPercussionType::none,"sustained voiced tone is rejected");
 d.reset();check(process(d,kick(0.25f)).type==VocalPercussionType::kick,"kick classification tolerates lower microphone gain");
 d.reset();std::mt19937 gen(4);std::uniform_real_distribution<float>dist(-0.001f,0.001f);std::vector<float>room(8192);for(auto&s:room)s=dist(gen);
 check(process(d,room).type==VocalPercussionType::none,"low-level room noise does not trigger");
 d.reset();check(d.process(nullptr,256).type==VocalPercussionType::none,"null input is safe");
 d.prepare(44100.0);check(process(d,kick()).type==VocalPercussionType::kick,"classification works at 44.1 kHz");
 return failures==0?0:1;
}
