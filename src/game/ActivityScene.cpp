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
    std::string dataPath = "assets/data/modules/" + moduleName() + ".json";
    m_modInfo = loadModuleInfo(dataPath);

    std::string bgPath = "assets/images/" + moduleName() + "/background.png";
    m_background = m_app.assets().loadTexture(bgPath);
    if (m_background) {
        SDL_QueryTexture(m_background, nullptr, nullptr, &m_bgWidth, nullptr);
        m_maxScrollX = m_bgWidth - 640;
        if (m_maxScrollX < 0) m_maxScrollX = 0;
    }

    loadOverlays();

    // Mark area as started in game state (only for main activity modules 1-6)
    if (m_moduleId >= 1 && m_moduleId <= 6) {
        m_app.gameState().startArea(m_moduleId);
    }

    // Ambient sound
    char ambPath[256];
    std::snprintf(ambPath, sizeof(ambPath), "assets/audio/ambient/AMBNT%d.WAV", m_moduleId);
    auto* ambient = m_app.assets().loadSound(ambPath);
    if (ambient) {
        m_app.audio().playAmbient(ambient);
    }

    m_scrollX = 0;
    m_activatedCount = 0;
    m_areaComplete = false;
    m_completionTimer = 0.0f;

    // Count required activations (type 1 interactive overlays with reasonable size)
    m_requiredActivations = 0;
    for (auto& ov : m_overlays) {
        if (isInteractive(ov)) {
            m_requiredActivations++;
        }
    }
}

void ActivityScene::onExit() {
    m_app.audio().stopAmbient();
}

bool ActivityScene::isInteractive(const SceneOverlay& ov) const {
    return ov.type == 1 && ov.w > 0 && ov.w < 640 && ov.h > 0 && ov.h < 480 && ov.clickable;
}

bool ActivityScene::isDraggable(const SceneOverlay& ov) const {
    // Type 0 small overlays (scenery items) can be dragged in decoration mode
    return ov.type == 0 && ov.w > 0 && ov.w < 200 && ov.h > 0 && ov.h < 200 && m_areaComplete;
}

void ActivityScene::loadOverlays() {
    m_overlays.clear();
    std::string modName = moduleName();

    for (auto& entry : m_modInfo.entries) {
        if (entry.frames == 0 && entry.width == 0) continue;

        SceneOverlay ov;
        ov.cx = entry.cx;
        ov.cy = entry.cy;
        ov.type = entry.type;
        ov.navModule = entry.navModule;
        ov.entryIndex = entry.index;

        if (entry.frames <= 1) {
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
            // In the original game, overlays with flag=8 are initially hidden
            // (they appear when triggered by game events like vehicle arrivals).
            // Type 3 overlays are UI popups, also initially hidden.
            int flag = 0;
            for (auto& e : m_modInfo.entries) {
                if (e.index == entry.index) { flag = e.flag; break; }
            }
            if (flag == 8 || flag == 12) {
                ov.visible = false;
                ov.clickable = false;
            }
            if (ov.type == 2 || ov.type == 3) {
                ov.visible = false;
                ov.clickable = false;
            }

            m_overlays.push_back(ov);
        }
    }
}

void ActivityScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT) {
            m_scrollX = std::max(0, m_scrollX - 32);
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            m_scrollX = std::min(m_maxScrollX, m_scrollX + 32);
        } else if (event.key.keysym.sym == SDLK_h || event.key.keysym.sym == SDLK_F1) {
            int helpId = helpModuleId();
            if (helpId > 0) {
                m_app.scenes().pushDeferred(
                    std::make_unique<ActivityScene>(m_app, helpId));
            }
        }
    }

    int mx = m_app.input().mouseX() + m_scrollX;
    int my = m_app.input().mouseY();

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Check for draggable overlay (decoration mode after completion)
        for (int i = static_cast<int>(m_overlays.size()) - 1; i >= 0; --i) {
            auto& ov = m_overlays[i];
            if (!isDraggable(ov) || !ov.visible) continue;
            SDL_Rect r = {ov.cx - ov.w / 2, ov.cy - ov.h / 2, ov.w, ov.h};
            SDL_Point pt = {mx, my};
            if (SDL_PointInRect(&pt, &r)) {
                m_draggedOverlay = i;
                m_dragOffX = ov.cx - mx;
                m_dragOffY = ov.cy - my;
                auto* grab = m_app.assets().loadSound("assets/audio/snds/CRANE1.WAV");
                if (grab) m_app.audio().playSound(grab);
                return;
            }
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_draggedOverlay >= 0) {
            auto* drop = m_app.assets().loadSound("assets/audio/snds/THUDSTON.WAV");
            if (drop) m_app.audio().playSound(drop);
            m_draggedOverlay = -1;
            return;
        }

        if (m_hoveredOverlay >= 0) {
            auto& ov = m_overlays[m_hoveredOverlay];

            if (isInteractive(ov) && !ov.activated) {
                // Activate this overlay - play its animation and mark done
                ov.activated = true;
                ov.currentFrame = 0;
                ov.frameTimer = 0.0f;
                m_activatedCount++;
                m_activeOverlay = m_hoveredOverlay;

                auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
                if (click) m_app.audio().playSound(click);

                checkCompletion();
            } else if (isInteractive(ov) && ov.activated) {
                // Already activated - toggle selection
                if (m_activeOverlay == m_hoveredOverlay) {
                    m_activeOverlay = -1;
                } else {
                    m_activeOverlay = m_hoveredOverlay;
                }
            }
        }
    }

    if (event.type == SDL_MOUSEMOTION && m_draggedOverlay >= 0) {
        auto& ov = m_overlays[m_draggedOverlay];
        ov.cx = mx + static_cast<int>(m_dragOffX);
        ov.cy = my + static_cast<int>(m_dragOffY);
        // Clamp to world
        ov.cx = std::max(ov.w / 2, std::min(m_bgWidth - ov.w / 2, ov.cx));
        ov.cy = std::max(ov.h / 2, std::min(480 - ov.h / 2, ov.cy));
    }
}

int ActivityScene::helpModuleId() const {
    switch (m_moduleId) {
        case  1: return 51;
        case  2: return 52;
        case  3: return 53;
        case  4: return 54;
        case  5: return 55;
        case  6: return 56;
        case  7: return 57;
        case  8: return 58;
        case  9: return 59;
        default: return 0;
    }
}

void ActivityScene::checkCompletion() {
    // Area is complete when enough interactive overlays have been activated
    // Use a fraction of total to be forgiving (some overlays are optional)
    int threshold = std::max(1, m_requiredActivations / 3);
    if (m_activatedCount >= threshold && !m_areaComplete) {
        m_areaComplete = true;
        m_completionTimer = 3.0f;

        if (m_moduleId >= 1 && m_moduleId <= 6) {
            m_app.gameState().completeArea(m_moduleId);
            m_app.gameState().save("tonka_save.dat");
        }

        auto* sfx = m_app.assets().loadSound("assets/audio/snds/HORN.WAV");
        if (sfx) m_app.audio().playSound(sfx);
    }
}

void ActivityScene::showCertificate() {
    int certModule = m_app.gameState().certificateModule(m_moduleId);
    if (certModule > 0) {
        m_app.scenes().pushDeferred(std::make_unique<ActivityScene>(m_app, certModule));
    }
}

void ActivityScene::update(float dt) {
    updateScroll(dt);

    // Animate overlays
    for (auto& ov : m_overlays) {
        if (!ov.animated || ov.frames.size() <= 1) continue;
        // Only auto-animate activated overlays or scenery (type 0)
        if (ov.type == 1 && !ov.activated) continue;

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

    if (m_draggedOverlay < 0) {
        // Check interactive overlays first (reverse order for top-most first)
        for (int i = static_cast<int>(m_overlays.size()) - 1; i >= 0; --i) {
            auto& ov = m_overlays[i];
            if (!ov.visible || ov.w == 0 || ov.h == 0) continue;
            if (!isInteractive(ov) && !isDraggable(ov)) continue;
            SDL_Rect rect = {ov.cx - ov.w / 2, ov.cy - ov.h / 2, ov.w, ov.h};
            SDL_Point pt = {mx, my};
            if (SDL_PointInRect(&pt, &rect)) {
                m_hoveredOverlay = i;
                break;
            }
        }
    }

    // Completion timer - show certificate after delay
    if (m_areaComplete && m_completionTimer > 0.0f) {
        m_completionTimer -= dt;
        if (m_completionTimer <= 0.0f) {
            showCertificate();
        }
    }
}

void ActivityScene::updateScroll(float /*dt*/) {
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
    // Background
    if (m_background) {
        SDL_Rect src = {m_scrollX, 0, 640, 480};
        SDL_Rect dst = {0, 0, 640, 480};
        renderer.drawTexture(m_background, &src, &dst);
    }

    // Overlays by layer: type 0 (scenery), then 1 (interactive), then 3 (UI)
    for (int pass = 0; pass <= 3; ++pass) {
        for (int i = 0; i < static_cast<int>(m_overlays.size()); ++i) {
            auto& ov = m_overlays[i];
            if (ov.type != pass) continue;
            if (!ov.visible || ov.w == 0) continue;
            if (ov.w >= 640 && ov.h >= 480) continue;

            SDL_Texture* tex = ov.texture;
            if (ov.animated && !ov.frames.empty()) {
                int frame = ov.currentFrame % static_cast<int>(ov.frames.size());
                tex = ov.frames[frame];
            }
            if (!tex) continue;

            int dx = ov.cx - ov.w / 2 - m_scrollX;
            int dy = ov.cy - ov.h / 2;
            if (dx + ov.w < 0 || dx > 640 || dy + ov.h < 0 || dy > 480) continue;

            renderer.drawTexture(tex, dx, dy);

            // Highlight hovered overlay
            if (i == m_hoveredOverlay) {
                SDL_Rect hr = {dx, dy, ov.w, ov.h};
                SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 30);
                SDL_RenderFillRect(renderer.sdlRenderer(), &hr);
            }

            // Show activation indicator on selected overlay
            if (i == m_activeOverlay) {
                SDL_Rect sr = {dx, dy, ov.w, ov.h};
                SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 180);
                SDL_RenderDrawRect(renderer.sdlRenderer(), &sr);
            }
        }
    }

    // Completion banner
    if (m_areaComplete && m_completionTimer > 0.0f) {
        SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
        SDL_Rect banner = {120, 200, 400, 80};
        SDL_SetRenderDrawColor(renderer.sdlRenderer(), 0, 0, 0, 180);
        SDL_RenderFillRect(renderer.sdlRenderer(), &banner);
        SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 204, 0, 255);
        SDL_RenderDrawRect(renderer.sdlRenderer(), &banner);
        m_app.text().drawTextCentered("Area Complete!", 220, 640, {255, 204, 0, 255}, 28);
        m_app.text().drawTextCentered("Certificate incoming...", 250, 640, {255, 255, 255, 255}, 16);
    }
}
