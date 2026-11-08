#include "PitchStabilityGuard.h"
#include <cmath>
#include <iostream>
namespace {
int failures=0,checks=0;
void check(bool ok,const char* s){++checks;if(ok)std::cout<<"[PASS] "<<s<<'\n';else{++failures;std::cerr<<"[FAIL] "<<s<<'\n';}}
PitchResult p(float cents,uint64_t seq){PitchResult r;r.frequencyHz=440.0f*std::pow(2.0f,cents/1200.0f);r.confidence=.95f;r.rms=.1f;r.voiced=true;r.sequence=seq;return r;}
void transient(){PitchStabilityGuard g;g.configure(true,40,30.0f);auto base=p(0,1);g.process(base);auto h=g.process(p(-75,2));check(h.voiced&&std::abs(h.frequencyHz-base.frequencyHz)<.01f,"Brief downward microdrop is held");auto back=p(-5,3);check(std::abs(g.process(back).frequencyHz-back.frequencyHz)<.01f,"Recovered pitch passes through");}
void persistent(){PitchStabilityGuard g;g.configure(true,40,30.0f);g.process(p(0,1));auto low=p(-120,2);check(std::abs(g.process(low).frequencyHz-440)<.01f,"First lower frame held");check(std::abs(g.process(p(-120,3)).frequencyHz-440)<.01f,"Second lower frame held");check(std::abs(g.process(p(-120,4)).frequencyHz-low.frequencyHz)<.01f,"Persistent lower note is accepted");}
void gaps(){PitchStabilityGuard g;g.configure(true,40,30.0f);auto base=p(0,1);g.process(base);PitchResult gap;gap.sequence=2;auto h=g.process(gap);check(h.voiced&&std::abs(h.frequencyHz-base.frequencyHz)<.01f,"Short unvoiced gap is bridged");g.process(gap);g.process(gap);check(!g.process(gap).voiced,"Persistent voice loss is not held indefinitely");}
void upward(){PitchStabilityGuard g;g.configure(true,40,30.0f);g.process(p(0,1));auto up=p(120,2);check(std::abs(g.process(up).frequencyHz-up.frequencyHz)<.01f,"Upward transition is immediate");g.configure(false,40,30.0f);auto down=p(-120,3);check(std::abs(g.process(down).frequencyHz-down.frequencyHz)<.01f,"Disabled guard is transparent");}
void timeBasedWindow()
{
    PitchStabilityGuard g;
    g.configure(true, 40.0f, 30.0f);
    const auto base = p(0, 1);
    g.process(base);
    for (uint64_t i = 2; i <= 6; ++i)
    {
        const auto held = g.process(p(-100, i), 0.005f);
        check(std::abs(held.frequencyHz - base.frequencyHz) < 0.01f,
              "Stability hold duration is independent of small audio blocks");
    }
    const auto accepted = g.process(p(-100, 7), 0.005f);
    check(std::abs(accepted.frequencyHz - p(-100, 7).frequencyHz) < 0.01f,
          "Persistent drop passes after 30 ms");
}
}
int main(){std::cout<<"NOVOX Pitch Stability Guard tests\n";transient();persistent();gaps();upward();timeBasedWindow();std::cout<<"\nChecks: "<<checks<<", failures: "<<failures<<'\n';return failures==0?0:1;}
