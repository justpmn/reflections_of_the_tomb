#include "Trap.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
void Trap::configureTimed(float safe,float warning,float extended,float offset) {
    if(!std::isfinite(safe)||!std::isfinite(warning)||!std::isfinite(extended)||!std::isfinite(offset)||safe<1.5f||warning<.75f||extended<.8f||offset<0)
        throw std::invalid_argument("Invalid timed spike durations");
    timed=true;safeDuration=safe;warningDuration=warning;extendedDuration=extended;initialPhase=offset;reset();
}
void Trap::activate(){active=true;disabled=false;}
void Trap::deactivate(){active=false;disabled=true;}
void Trap::reset(){active=true;disabled=false;phase=std::fmod(initialPhase,safeDuration+warningDuration+extendedDuration);extensionAmount=timed?0.f:1.f;evaluate();}
Trap::State Trap::state() const {
    if(disabled||!active)return State::Retracted;
    if(!timed)return State::Extended;
    if(phase<safeDuration)return State::Retracted;
    if(phase<safeDuration+warningDuration)return State::Warning;
    return State::Extended;
}
void Trap::evaluate(){
    if(!timed||disabled)return;
    extensionAmount=0;
    if(state()==State::Extended){
        const double elapsed=phase-safeDuration-warningDuration;
        const double t=std::clamp(std::min(elapsed,extendedDuration-elapsed)/.4,0.,1.);
        extensionAmount=float(t*t*(3-2*t));
    }
}
bool Trap::isActive()const{return active&&!disabled&&(!timed||(state()==State::Extended&&extensionAmount>.18f));}
void Trap::update(float seconds){
    if(!std::isfinite(seconds)||seconds<=0)return;
    if(timed&&!disabled){phase=std::fmod(phase+double(seconds),safeDuration+warningDuration+extendedDuration);evaluate();}
    else extensionAmount=std::clamp(extensionAmount+(active&&!disabled?1.f:-1.f)*seconds*2.f,0.f,1.f);
}
double Trap::safeTimeRemaining()const {if(disabled)return 1e9;if(!timed)return active?0:1e9;return state()==State::Retracted?safeDuration-phase:0;}
bool Trap::safeToCross(double seconds)const{return safeTimeRemaining()>seconds;}
