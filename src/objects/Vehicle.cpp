#include "Vehicle.h"
#include <cmath>

void Vehicle::setPath(const std::vector<Waypoint>& path) {
    m_path = path;
    // Build departure path (reverse)
    m_departurePath.assign(path.rbegin(), path.rend());
}

void Vehicle::startDriving() {
    if (m_path.empty()) return;
    m_state = State::Driving;
    m_pathIndex = 0;
    m_x = m_path[0].x;
    m_y = m_path[0].y;
    play();
}

void Vehicle::startDeparting() {
    if (m_departurePath.empty()) {
        m_state = State::Idle;
        return;
    }
    m_state = State::Departing;
    m_pathIndex = 0;
    play();
}

void Vehicle::setActive(bool active) {
    if (active) {
        m_state = State::Active;
        stop(); // Stop driving animation, player controls now
    } else {
        m_state = State::Idle;
    }
}

void Vehicle::update(float dt) {
    GameObject::update(dt);

    switch (m_state) {
        case State::Driving:
        case State::Departing:
            followPath(dt);
            break;
        default:
            break;
    }
}

void Vehicle::followPath(float dt) {
    auto& path = (m_state == State::Departing) ? m_departurePath : m_path;

    if (m_pathIndex + 1 >= static_cast<int>(path.size())) {
        // Reached end of path
        if (m_state == State::Driving) {
            m_state = State::Active;
            stop();
            if (m_onArrived) m_onArrived();
        } else {
            m_state = State::Idle;
            stop();
            setVisible(false);
        }
        return;
    }

    float tx = path[m_pathIndex + 1].x;
    float ty = path[m_pathIndex + 1].y;

    float dx = tx - m_x;
    float dy = ty - m_y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f) {
        m_pathIndex++;
        return;
    }

    float step = m_speed * dt;
    if (step >= dist) {
        m_x = tx;
        m_y = ty;
        m_pathIndex++;
    } else {
        m_x += (dx / dist) * step;
        m_y += (dy / dist) * step;
    }
}
