#pragma once
#include <algorithm>
enum class InputChannelMode:int{automatic=0,left=1,right=2,stereoMix=3};
inline int chooseInputChannel(InputChannelMode mode,const float*const* input,int n,int samples)noexcept{
 if(!input||n<=0)return -1;auto first=[&](){for(int c=0;c<n;++c)if(input[c])return c;return -1;};
 if(mode==InputChannelMode::stereoMix)return -1;if(mode==InputChannelMode::left)return input[0]?0:first();if(mode==InputChannelMode::right)return n>1&&input[1]?1:first();
 int best=-1;double bestE=-1;for(int c=0;c<n;++c){if(!input[c]||samples<=0)continue;double e=0;for(int i=0;i<samples;++i){double v=input[c][i];e+=v*v;}if(e>bestE){bestE=e;best=c;}}return best>=0?best:first();
}