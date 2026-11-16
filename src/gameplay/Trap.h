#pragma once
// Each trap owns its cycle; no shared wall-clock timer.
class Trap {
public:
    enum class State { Retracted, Warning, Extended };
    Trap() = default;
    void configureTimed(float safe, float warning, float extended, float offset);
    void activate();
    void deactivate();
    void reset();
    bool isActive() const;
    bool isTimed() const { return timed; }
    bool isDisabled() const { return disabled; }
    State state() const;
    void update(float seconds);
    float extension() const { return extensionAmount; }
    double cyclePosition() const { return phase; }
    double safeTimeRemaining() const;
    // Enough time to cross a whole cell including the Ranger radius.
    bool safeToCross(double seconds) const;
private:
    void evaluate();
    bool active=true, timed=false, disabled=false;
    float extensionAmount=1;
    double safeDuration=5, warningDuration=1, extendedDuration=2;
    double initialPhase=0, phase=0;
};
