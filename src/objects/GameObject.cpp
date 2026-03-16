#include "GameObject.h"
#include "engine/Renderer.h"
#include "engine/AssetManager.h"
#include <cstdio>

bool GameObject::loadFrames(AssetManager& assets, const std::string& basePath, int count) {
    m_frames.clear();
    for (int i = 0; i < count; ++i) {
        char path[512];
        std::snprintf(path, sizeof(path), "%s_frame%03d.png", basePath.c_str(), i);
        auto* tex = assets.loadTexture(path);
        if (tex) {
            m_frames.push_back(tex);
        }
    }
    if (!m_frames.empty()) {
        SDL_QueryTexture(m_frames[0], nullptr, nullptr, &m_width, &m_height);
        return true;
    }
    return false;
}

bool GameObject::loadSingleFrame(AssetManager& assets, const std::string& path) {
    m_frames.clear();
    auto* tex = assets.loadTexture(path);
    if (tex) {
        m_frames.push_back(tex);
        SDL_QueryTexture(tex, nullptr, nullptr, &m_width, &m_height);
        return true;
    }
    return false;
}

void GameObject::update(float dt) {
    if (!m_playing || m_frames.size() <= 1) return;

    m_frameTimer += dt;
    float interval = 1.0f / m_frameRate;
    while (m_frameTimer >= interval) {
        m_frameTimer -= interval;
        m_currentFrame++;
        if (m_currentFrame >= static_cast<int>(m_frames.size())) {
            if (m_looping) {
                m_currentFrame = 0;
            } else {
                m_currentFrame = static_cast<int>(m_frames.size()) - 1;
                m_playing = false;
            }
        }
    }
}

void GameObject::render(Renderer& renderer, int scrollX) {
    if (!m_visible || m_frames.empty()) return;

    int frame = m_currentFrame % static_cast<int>(m_frames.size());
    int dx = static_cast<int>(m_x) - m_width / 2 - scrollX;
    int dy = static_cast<int>(m_y) - m_height / 2;

    // Cull off-screen
    if (dx + m_width < 0 || dx > 640 || dy + m_height < 0 || dy > 480) return;

    renderer.drawTexture(m_frames[frame], dx, dy);
}

bool GameObject::hitTest(int worldX, int worldY) const {
    if (!m_visible || m_frames.empty()) return false;
    SDL_Rect r = worldRect();
    SDL_Point pt = {worldX, worldY};
    return SDL_PointInRect(&pt, &r) == SDL_TRUE;
}

SDL_Rect GameObject::worldRect() const {
    return {
        static_cast<int>(m_x) - m_width / 2,
        static_cast<int>(m_y) - m_height / 2,
        m_width,
        m_height
    };
}
