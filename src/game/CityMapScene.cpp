#include "CityMapScene.h"
#include "engine/Application.h"
#include "game/ActivityScene.h"
#include <cstdio>

CityMapScene::CityMapScene(Application& app)
    : m_app(app) {}

void CityMapScene::onEnter() {
    m_background = m_app.assets().loadTexture("assets/images/module11/background.png");
    loadSites();

    auto* ambient = m_app.assets().loadSound("assets/audio/ambient/AMBNT11.WAV");
    if (ambient) {
        m_app.audio().playAmbient(ambient);
    }
}

void CityMapScene::onExit() {
    m_app.audio().stopAmbient();
}

void CityMapScene::loadSites() {
    m_sites.clear();

    // 3 sites from MODULE11 data. Each site has overlays for Park(4), Castle(5), HiRise(6).
    // We show the overlay matching the assigned area. Positions from entry headers.
    // Site 0 (entries 0-3): top area, centered around (108,93)
    // Site 1 (entries 4-7): bottom-left, centered around (197,355)
    // Site 2 (entries 8-11): right side, centered around (476,211)
    struct SiteDef {
        int overlayBase;  // base overlay index (0, 4, or 8)
        int moduleId;     // which city area this site is for
        int cx, cy;
    };

    // Assign each site to a different city area
    SiteDef defs[] = {
        {0, 4, 108, 93},   // Site 0 -> Park
        {4, 5, 197, 355},  // Site 1 -> Castle
        {8, 6, 476, 211},  // Site 2 -> Hi-Rise
    };

    for (auto& def : defs) {
        Site site;
        site.moduleId = def.moduleId;
        site.cx = def.cx;
        site.cy = def.cy;

        // Pick the overlay that matches this site's module ID
        // Entries at base+0=Park(4), base+1=Castle(5), base+2=HiRise(6), base+3=empty
        int ovIdx = def.overlayBase;
        if (def.moduleId == 4) ovIdx = def.overlayBase + 0;
        else if (def.moduleId == 5) ovIdx = def.overlayBase + 1;
        else if (def.moduleId == 6) ovIdx = def.overlayBase + 2;

        char path[256];
        std::snprintf(path, sizeof(path),
            "assets/images/module11/overlay_%03d_frame000.png", ovIdx);
        site.texture = m_app.assets().loadTexture(path);
        if (!site.texture) {
            std::snprintf(path, sizeof(path),
                "assets/images/module11/overlay_%03d.png", ovIdx);
            site.texture = m_app.assets().loadTexture(path);
        }

        if (site.texture) {
            SDL_QueryTexture(site.texture, nullptr, nullptr, &site.w, &site.h);
        } else {
            site.w = 200;
            site.h = 200;
        }

        m_sites.push_back(site);
    }
}

void CityMapScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_hoveredSite >= 0) {
            int moduleId = m_sites[m_hoveredSite].moduleId;
            auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
            if (click) m_app.audio().playSound(click);
            m_app.scenes().pushDeferred(std::make_unique<ActivityScene>(m_app, moduleId));
        }
    }
}

void CityMapScene::update(float /*dt*/) {
    int mx = m_app.input().mouseX();
    int my = m_app.input().mouseY();

    m_hoveredSite = -1;
    for (int i = 0; i < static_cast<int>(m_sites.size()); ++i) {
        auto& s = m_sites[i];
        SDL_Rect r = {s.cx - s.w / 2, s.cy - s.h / 2, s.w, s.h};
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &r)) {
            m_hoveredSite = i;
            break;
        }
    }
}

void CityMapScene::render(Renderer& renderer) {
    if (m_background) {
        renderer.drawTextureEx(m_background, 0, 0, 640, 480);
    }

    for (int i = 0; i < static_cast<int>(m_sites.size()); ++i) {
        auto& s = m_sites[i];
        if (s.texture) {
            int dx = s.cx - s.w / 2;
            int dy = s.cy - s.h / 2;
            renderer.drawTexture(s.texture, dx, dy);
        }

        if (i == m_hoveredSite) {
            SDL_Rect r = {s.cx - s.w / 2, s.cy - s.h / 2, s.w, s.h};
            SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 40);
            SDL_RenderFillRect(renderer.sdlRenderer(), &r);
        }
    }

    // Area labels
    const char* labels[] = {"Park", "Castle", "Hi-Rise"};
    for (int i = 0; i < static_cast<int>(m_sites.size()); ++i) {
        auto& s = m_sites[i];
        m_app.text().drawTextCentered(labels[i], s.cy + s.h / 2 + 4, s.cx * 2,
                                       {255, 255, 255, 255}, 14);
    }
}
