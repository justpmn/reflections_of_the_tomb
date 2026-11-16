#pragma once
// Require a release after countdown, transition or death before accepting held inputs.
class GameplayInputGate {
    bool releaseRequired=true;
public:
    bool allow(bool enabled,bool anyHeld) {
        if(!enabled){releaseRequired=anyHeld;return false;}
        if(releaseRequired){if(anyHeld)return false;releaseRequired=false;}
        return true;
    }
};
