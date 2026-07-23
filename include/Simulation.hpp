#pragma once

// Owns the simulation clock. `time()` is what drives the accretion disk's
// rotation; it is measured in units of M (G = c = 1), i.e. the same units as
// the coordinate time of a distant observer.
class Simulation {
public:
    void update(double realDeltaSeconds);

    void reset();
    void togglePause();
    void multiplySpeed(double factor);

    double time()   const { return m_time; }
    double speed()  const { return m_speed; }
    bool   paused() const { return m_paused; }

    void setTime(double t) { m_time = t; }

    // How many units of M elapse per real-time second at speed 1.0.
    double baseRate = 6.0;

    double minSpeed = 0.02;
    double maxSpeed = 200.0;

private:
    double m_time   = 0.0;
    double m_speed  = 1.0;
    bool   m_paused = false;
};
