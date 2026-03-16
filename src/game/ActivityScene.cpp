#include "ActivityScene.h"
#include "engine/Application.h"
#include <cstdio>
#include <algorithm>
#include <iostream>

ActivityScene::ActivityScene(Application& app, int moduleId)
    : m_app(app), m_moduleId(moduleId) {}

std::string ActivityScene::moduleName() const {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "module%02d", m_moduleId);
    return buf;
}

void ActivityScene::onEnter() {
    // Load module position data
    std::string dataPath = "assets/data/modules/" + moduleName() + ".json";
    m_modInfo = loadModuleInfo(dataPath);

    // Load background
    std::string bgPath = "assets/images/" + moduleName() + "/background.png";
    m_background = m_app.assets().loadTexture(bgPath);
    if (m_background) {
        SDL_QueryTexture(m_background, nullptr, nullptr, &m_bgWidth, nullptr);
        m_maxScrollX = m_bgWidth - 640;
        if (m_maxScrollX < 0) m_maxScrollX = 0;
    }

    loadOverlays();

    // Ambient sound
    char ambPath[256];
    std::snprintf(ambPath, sizeof(ambPath), "assets/audio/ambient/AMBNT%d.WAV", m_moduleId);
    auto* ambient = m_app.assets().loadSound(ambPath);
    if (ambient) {
        m_app.audio().playAmbient(ambient);
    }

    m_scrollX = 0;
}

void ActivityScene::onExit() {
    m_app.audio().stopAmbient();
}

void ActivityScene::loadOverlays() {
    m_overlays.clear();
    std::string modName = moduleName();

    for (auto& entry : m_modInfo.entries) {
        // Skip entries with no frames or zero dimensions
        if (entry.frames == 0 && entry.width == 0) continue;

        SceneOverlay ov;
        ov.cx = entry.cx;
        ov.cy = entry.cy;
        ov.type = entry.type;
        ov.navModule = entry.navModule;

        if (entry.frames <= 1) {
            // Static overlay - try loading single image
            char path[256];
            std::snprintf(path, sizeof(path),
                "assets/images/%s/overlay_%03d.png", modName.c_str(), entry.index);
            ov.texture = m_app.assets().loadTexture(path);
            if (!ov.texture) {
                std::snprintf(path, sizeof(path),
                    "assets/images/%s/overlay_%03d_frame000.png", modName.c_str(), entry.index);
                ov.texture = m_app.assets().loadTexture(path);
            }
            if (ov.texture) {
                SDL_QueryTexture(ov.texture, nullptr, nullptr, &ov.w, &ov.h);
            }
            ov.animated = false;
        } else {
            // Animated overlay - load all frames
            for (int f = 0; f < entry.frames; ++f) {
                char path[256];
                std::snprintf(path, sizeof(path),
                    "assets/images/%s/overlay_%03d_frame%03d.png",
                    modName.c_str(), entry.index, f);
                auto* tex = m_app.assets().loadTexture(path);
                if (tex) {
                    ov.frames.push_back(tex);
                }
            }
            if (!ov.frames.empty()) {
                ov.texture = ov.frames[0];
                SDL_QueryTexture(ov.texture, nullptr, nullptr, &ov.w, &ov.h);
                ov.animated = true;
            }
        }

        if (ov.texture || !ov.frames.empty()) {
            m_overlays.push_back(ov);
        }
    }

    std::cout << moduleName() << ": loaded " << m_overlays.size()
              << " overlays (bg " << m_bgWidth << "x480)\n";
}

void ActivityScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT) {
            m_scrollX = std::max(0, m_scrollX - 32);
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            m_scrollX = std::min(m_maxScrollX, m_scrollX + 32);
        } else if (event.key.keysym.sym == SDLK_h || event.key.keysym.sym == SDLK_F1) {
            // Open help screen for this area
            int helpId = helpModuleId();
            if (helpId > 0) {
                m_app.scenes().pushDeferred(
                    std::make_unique<ActivityScene>(m_app, helpId));
            }
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_hoveredOverlay >= 0) {
            auto& ov = m_overlays[m_hoveredOverlay];
            // Toggle animation for interactive overlays
            if (ov.animated && ov.type == 1) {
                ov.currentFrame = 0;
                ov.frameTimer = 0.0f;
            }
            auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
            if (click) m_app.audio().playSound(click);
        }
    }
}

int ActivityScene::helpModuleId() const {
    // Map activity modules to their help screen modules
    switch (m_moduleId) {
        case  1: return 51;  // Quarry help
        case  2: return 52;  // Avalanche help
        case  3: return 53;  // Desert help
        case  4: return 54;  // City Park help
        case  5: return 55;  // City Castle help
        case  6: return 56;  // City Hi-Rise help
        case  7: return 57;  // Monster truck help
        case  8: return 58;  // Dozer help
        case  9: return 59;  // Grader help
        default: return 0;
    }
}

void ActivityScene::update(float dt) {
    updateScroll(dt);

    // Animate overlays
    for (auto& ov : m_overlays) {
        if (!ov.animated || ov.frames.size() <= 1) continue;
        ov.frameTimer += dt;
        float rate = 8.0f;
        if (ov.frameTimer >= 1.0f / rate) {
            ov.frameTimer -= 1.0f / rate;
            ov.currentFrame = (ov.currentFrame + 1) % static_cast<int>(ov.frames.size());
        }
    }

    // Hover detection
    int mx = m_app.input().mouseX() + m_scrollX;
    int my = m_app.input().mouseY();
    m_hoveredOverlay = -1;

    for (int i = 0; i < static_cast<int>(m_overlays.size()); ++i) {
        auto& ov = m_overlays[i];
        if (ov.w == 0 || ov.h == 0) continue;
        // Only highlight interactive (type 1) overlays
        if (ov.type != 1) continue;
        SDL_Rect rect = {ov.cx - ov.w / 2, ov.cy - ov.h / 2, ov.w, ov.h};
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &rect)) {
            m_hoveredOverlay = i;
            break;
        }
    }
}

void ActivityScene::updateScroll(float dt) {
    if (m_maxScrollX <= 0) return;

    int mx = m_app.input().mouseX();
    const int edgeZone = 40;
    const int scrollSpeed = 4;

    if (mx < edgeZone && m_scrollX > 0) {
        m_scrollX = std::max(0, m_scrollX - scrollSpeed);
    } else if (mx > 640 - edgeZone && m_scrollX < m_maxScrollX) {
        m_scrollX = std::min(m_maxScrollX, m_scrollX + scrollSpeed);
    }
}

void ActivityScene::render(Renderer& renderer) {
    // Background (scrolled for wide backgrounds)
    if (m_background) {
        SDL_Rect src = {m_scrollX, 0, 640, 480};
        SDL_Rect dst = {0, 0, 640, 480};
        renderer.drawTexture(m_background, &src, &dst);
    }

    // Overlays sorted by type: type 0 (scenery) first, then type 1 (interactive)
    for (int pass = 0; pass <= 3; ++pass) {
        for (int i = 0; i < static_cast<int>(m_overlays.size()); ++i) {
            auto& ov = m_overlays[i];
            if (ov.type != pass) continue;
            if (!ov.visible || ov.w == 0) continue;

            // Skip full-screen overlays (640x480) in normal rendering -
            // these are typically popup menus or special screens
            if (ov.w >= 640 && ov.h >= 480) continue;

            SDL_Texture* tex = ov.texture;
            if (ov.animated && !ov.frames.empty()) {
                int frame = ov.currentFrame % static_cast<int>(ov.frames.size());
                tex = ov.frames[frame];
            }
            if (!tex) continue;

            int dx = ov.cx - ov.w / 2 - m_scrollX;
            int dy = ov.cy - ov.h / 2;

            // Cull off-screen
            if (dx + ov.w < 0 || dx > 640 || dy + ov.h < 0 || dy > 480) continue;

            renderer.drawTexture(tex, dx, dy);

            // Highlight hovered interactive overlay
            if (i == m_hoveredOverlay) {
                SDL_Rect hr = {dx, dy, ov.w, ov.h};
                SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 30);
                SDL_RenderFillRect(renderer.sdlRenderer(), &hr);
            }
        }
    }
}
