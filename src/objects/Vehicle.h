#pragma once

#include "GameObject.h"
#include <vector>
#include <functional>

struct Waypoint {
    float x, y;
};

class Vehicle : public GameObject {
public:
    enum class State {
        Idle,       // Parked, waiting for player
        Driving,    // Following a path
        Active,     // Player is controlling this vehicle
        Departing,  // Driving away after use
    };

    void setPath(const std::vector<Waypoint>& path);
    void startDriving();
    void startDeparting();
    void setActive(bool active);

    State state() const { return m_state; }
    bool isIdle() const { return m_state == State::Idle; }
    bool isDriving() const { return m_state == State::Driving; }
    bool isActive() const { return m_state == State::Active; }

    void setSpeed(float pixelsPerSec) { m_speed = pixelsPerSec; }
    void setOnArrived(std::function<void()> cb) { m_onArrived = cb; }

    void update(float dt) override;

private:
    State m_state = State::Idle;
    std::vector<Waypoint> m_path;
    std::vector<Waypoint> m_departurePath;
    int m_pathIndex = 0;
    float m_speed = 120.0f;
    std::function<void()> m_onArrived;

    void followPath(float dt);
};
