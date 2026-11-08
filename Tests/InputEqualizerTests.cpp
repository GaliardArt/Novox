#include "InputEqualizer.h"
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    constexpr double rate = 48000.0;
    constexpr double pi = 3.14159265358979323846;
    std::vector<float> signal(48000);
    for (size_t i=0;i<signal.size();++i)
        signal[i]=0.1f*static_cast<float>(std::sin(2.0*pi*1000.0*static_cast<double>(i)/rate));

    auto bypass=signal;
    InputEqualizer eq;
    eq.prepare(rate);
    eq.process(bypass.data(),static_cast<int>(bypass.size()));
    float bypassError=0.0f;
    for(size_t i=0;i<signal.size();++i)bypassError=std::max(bypassError,std::abs(signal[i]-bypass[i]));
    if(bypassError>1.0e-7f){std::cerr<<"EQ disabled state must be transparent\n";return 1;}

    eq.setGains(0.0f,6.0f,0.0f);
    eq.setEnabled(true);
    auto processed=signal;
    eq.process(processed.data(),static_cast<int>(processed.size()));
    double inputEnergy=0.0,outputEnergy=0.0;
    for(size_t i=2400;i<signal.size();++i){inputEnergy+=signal[i]*signal[i];outputEnergy+=processed[i]*processed[i];}
    const double gain=std::sqrt(outputEnergy/inputEnergy);
    if(!(gain>1.7&&gain<2.3)){std::cerr<<"Mid-band boost did not apply expected gain: "<<gain<<"\n";return 1;}
    std::cout<<"Input EQ bypass and mid-band gain tests passed\n";
    return 0;
}
