#include "MainMapScene.h"
#include "engine/Application.h"
#include "game/GarageScene.h"
#include "game/QuarryScene.h"
#include "game/CityMapScene.h"
#include "game/TimeCardScene.h"
#include "game/ActivityScene.h"
#include <iostream>
#include <cstdio>

// Overlay position data extracted from MODULE10.DAT entry headers.
// Fields: overlay_index, center_x, center_y, type, module_id
static const struct {
    int index;
    int cx, cy;
    int type;
    int moduleId;
} kOverlayData[] = {
    // Type 1: Navigation arrows/indicators (clickable, navigate to module)
    { 0, 235, 305, 1,  1},  // Quarry
    { 1, 474,  91, 1,  2},  // Avalanche
    { 2,  67, 235, 1,  3},  // Desert
    { 3, 494, 209, 1, 11},  // City
    { 4, 557, 172, 1, 11},  // City (alt)
    { 5, 444, 155, 1, 11},  // City (alt)
    // Type 0: Area preview images (decorative, placed on map)
    {38, 226, 274, 0,  1},  // Quarry area
    {39, 433,  96, 0,  2},  // Avalanche area
    {40, 134, 195, 0,  3},  // Desert area
    {41, 512, 197, 0, 11},  // City area
    {42, 466, 378, 0, 12},  // Garage area
    // Type 1: Options/sign
    {43,  73, 401, 1, 13},  // Options sign
};

MainMapScene::MainMapScene(Application& app)
    : m_app(app) {}

void MainMapScene::onEnter() {
    m_background = m_app.assets().loadTexture("assets/images/module10/background.png");
    if (!m_background) {
        std::cerr << "Failed to load main map background\n";
    }

    loadOverlays();

    // Ambient sound
    auto* ambient = m_app.assets().loadSound("assets/audio/ambient/AMBNT10.WAV");
    if (ambient) {
        m_app.audio().playAmbient(ambient);
    }
}

void MainMapScene::onExit() {
    m_app.audio().stopAmbient();
}

void MainMapScene::loadOverlays() {
    m_overlays.clear();

    for (const auto& od : kOverlayData) {
        char path[256];
        std::snprintf(path, sizeof(path), "assets/images/module10/overlay_%03d.png", od.index);

        MapOverlay ov;
        ov.texture = m_app.assets().loadTexture(path);
        ov.cx = od.cx;
        ov.cy = od.cy;
        ov.type = od.type;
        ov.moduleId = od.moduleId;

        if (ov.texture) {
            SDL_QueryTexture(ov.texture, nullptr, nullptr, &ov.w, &ov.h);
        } else {
            ov.w = ov.h = 0;
        }

        m_overlays.push_back(ov);
    }
}

void MainMapScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_hoveredNav >= 0) {
            navigateTo(m_overlays[m_hoveredNav].moduleId);
        }
    }
}

void MainMapScene::update(float /*dt*/) {
    int mx = m_app.input().mouseX();
    int my = m_app.input().mouseY();

    m_hoveredNav = -1;
    for (int i = 0; i < static_cast<int>(m_overlays.size()); ++i) {
        auto& ov = m_overlays[i];
        // Only type 0 (area images) and type 1 (nav) are clickable
        if (ov.type != 0 && ov.type != 1) continue;
        if (!ov.texture) continue;

        SDL_Rect rect = {ov.cx - ov.w / 2, ov.cy - ov.h / 2, ov.w, ov.h};
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &rect)) {
            m_hoveredNav = i;
            break;
        }
    }
}

void MainMapScene::render(Renderer& renderer) {
    // Draw background
    if (m_background) {
        renderer.drawTextureEx(m_background, 0, 0, 640, 480);
    }

    // Draw type 0 overlays (area images) first
    for (auto& ov : m_overlays) {
        if (ov.type != 0 || !ov.texture) continue;
        int dx = ov.cx - ov.w / 2;
        int dy = ov.cy - ov.h / 2;
        renderer.drawTexture(ov.texture, dx, dy);
    }

    // Draw type 1 overlays (nav elements)
    for (auto& ov : m_overlays) {
        if (ov.type != 1 || !ov.texture) continue;
        int dx = ov.cx - ov.w / 2;
        int dy = ov.cy - ov.h / 2;
        renderer.drawTexture(ov.texture, dx, dy);
    }

    // Highlight hovered element
    if (m_hoveredNav >= 0) {
        auto& ov = m_overlays[m_hoveredNav];
        SDL_Rect rect = {ov.cx - ov.w / 2, ov.cy - ov.h / 2, ov.w, ov.h};
        SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 40);
        SDL_RenderFillRect(renderer.sdlRenderer(), &rect);
    }
}

void MainMapScene::navigateTo(int moduleId) {
    auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
    if (click) {
        m_app.audio().playSound(click);
    }

    switch (moduleId) {
        case 1:
            m_app.scenes().pushDeferred(std::make_unique<QuarryScene>(m_app));
            break;
        case 11:
            m_app.scenes().pushDeferred(std::make_unique<CityMapScene>(m_app));
            break;
        case 12:
            m_app.scenes().pushDeferred(std::make_unique<GarageScene>(m_app));
            break;
        case 13:
            m_app.scenes().pushDeferred(std::make_unique<TimeCardScene>(m_app));
            break;
        default:
            m_app.scenes().pushDeferred(std::make_unique<ActivityScene>(m_app, moduleId));
            break;
    }
}
