#include "PercussionEventQueue.h"
#include <iostream>
int main(){
 int failures=0;auto check=[&failures](bool ok,const char*name){std::cout<<(ok?"[PASS] ":"[FAIL] ")<<name<<'\n';if(!ok)++failures;};
 PercussionEventQueue q;VocalPercussionResult e;
 check(!q.pop(e),"empty queue returns false");
 VocalPercussionResult a{VocalPercussionType::kick,0.7f,0.2f,1},b{VocalPercussionType::snare,0.8f,0.3f,2};
 check(q.push(a)&&q.push(b),"queue accepts events");
 check(q.pop(e)&&e.type==VocalPercussionType::kick&&e.sequence==1,"queue preserves first event and order");
 check(q.pop(e)&&e.type==VocalPercussionType::snare&&e.sequence==2,"queue preserves second event and payload");
 for(size_t i=0;i<PercussionEventQueue::capacity-1;++i)check(q.push(a),"queue fills to bounded capacity");
 check(!q.push(a)&&q.getDropped()==1,"overflow is counted without blocking");
 check(q.pop(e),"queue recovers after consumer advances");
 check(q.push(b),"queue accepts data after recovery");
 return failures==0?0:1;
}
