#include "TimeCardScene.h"
#include "engine/Application.h"
#include "game/ActivityScene.h"
#include <cstdio>

TimeCardScene::TimeCardScene(Application& app)
    : m_app(app) {}

void TimeCardScene::onEnter() {
    m_background = m_app.assets().loadTexture("assets/images/module13/background.png");
    loadAssets();

    auto* ambient = m_app.assets().loadSound("assets/audio/ambient/AMBNT13.WAV");
    if (ambient) {
        m_app.audio().playAmbient(ambient);
    }
}

void TimeCardScene::onExit() {
    m_app.audio().stopAmbient();
}

void TimeCardScene::loadAssets() {
    m_cells.clear();
    m_buttons.clear();

    // Time card area cells from MODULE13 entry data.
    // Entries 6-13 are the completion stamp cells, arranged in a 3x2 grid + extras.
    // Each has 2 frames: frame 0 = complete stamp, frame 1 = empty.
    // Layout (from entry positions):
    //   Row 1: entry 8 (Quarry,pos 148,145), entry 11 (Avalanche,pos 484,144)
    //   Row 2: entry 7 (Desert,pos 149,223), entry 10 (Park,pos 487,223)
    //   Row 3: entry 6 (Castle,pos 150,306), entry 9 (HiRise,pos 486,300)
    //   Bottom row: entry 12 (Garage,pos 320,307), entry 13 (Master,pos 316,184)
    struct CellDef {
        int overlayIdx;
        int cx, cy;
        int moduleId;
    };
    CellDef cellDefs[] = {
        { 8, 148, 145, 1},  // Quarry
        {11, 484, 144, 2},  // Avalanche
        { 7, 149, 223, 3},  // Desert
        {10, 487, 223, 4},  // Park
        { 6, 150, 306, 5},  // Castle
        { 9, 486, 300, 6},  // HiRise
        {12, 320, 307, 12}, // Garage
        {13, 316, 184, 0},  // Master Builder (special)
    };

    for (auto& cd : cellDefs) {
        Cell cell;
        cell.cx = cd.cx;
        cell.cy = cd.cy;
        cell.moduleId = cd.moduleId;

        char path[256];
        std::snprintf(path, sizeof(path),
            "assets/images/module13/overlay_%03d_frame000.png", cd.overlayIdx);
        cell.complete = m_app.assets().loadTexture(path);

        std::snprintf(path, sizeof(path),
            "assets/images/module13/overlay_%03d_frame001.png", cd.overlayIdx);
        cell.incomplete = m_app.assets().loadTexture(path);

        if (!cell.complete) {
            std::snprintf(path, sizeof(path),
                "assets/images/module13/overlay_%03d.png", cd.overlayIdx);
            cell.complete = m_app.assets().loadTexture(path);
        }

        SDL_Texture* measure = cell.complete ? cell.complete : cell.incomplete;
        if (measure) {
            SDL_QueryTexture(measure, nullptr, nullptr, &cell.w, &cell.h);
        } else {
            cell.w = 160;
            cell.h = 70;
        }

        m_cells.push_back(cell);
    }

    // Buttons
    // Entry 0: QUIT at (203,414)
    {
        Button btn;
        btn.action = Button::Quit;
        btn.cx = 203; btn.cy = 414;
        char path[256];
        std::snprintf(path, sizeof(path), "assets/images/module13/overlay_000_frame000.png");
        btn.texture = m_app.assets().loadTexture(path);
        if (btn.texture) {
            SDL_QueryTexture(btn.texture, nullptr, nullptr, &btn.w, &btn.h);
        } else { btn.w = 92; btn.h = 88; }
        m_buttons.push_back(btn);
    }

    // Entry 35: CANCEL at (314,418) - not always present, use entry 2 if available
    {
        Button btn;
        btn.action = Button::Cancel;
        btn.cx = 314; btn.cy = 418;
        btn.texture = m_app.assets().loadTexture("assets/images/module13/overlay_035.png");
        if (btn.texture) {
            SDL_QueryTexture(btn.texture, nullptr, nullptr, &btn.w, &btn.h);
        } else { btn.w = 240; btn.h = 85; }
        m_buttons.push_back(btn);
    }

    // Name header (entry 21)
    m_nameHeader = m_app.assets().loadTexture("assets/images/module13/overlay_021.png");
    m_nameHdrX = 313;
    m_nameHdrY = 59;
}

void TimeCardScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_hoveredButton >= 0) {
            auto& btn = m_buttons[m_hoveredButton];
            auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
            if (click) m_app.audio().playSound(click);

            if (btn.action == Button::Quit) {
                m_app.quit();
            } else if (btn.action == Button::Cancel) {
                m_app.scenes().popDeferred();
            }
            return;
        }

        if (m_hoveredCell >= 0) {
            auto& cell = m_cells[m_hoveredCell];
            // If completed, show certificate
            if (cell.moduleId > 0 && m_app.gameState().isAreaComplete(cell.moduleId)) {
                int certMod = m_app.gameState().certificateModule(cell.moduleId);
                if (certMod > 0) {
                    auto* click = m_app.assets().loadSound("assets/audio/snds/PAGEOPEN.WAV");
                    if (click) m_app.audio().playSound(click);
                    m_app.scenes().pushDeferred(std::make_unique<ActivityScene>(m_app, certMod));
                }
            } else if (cell.moduleId == 0) {
                // Master Builder cell - check if all areas complete
                if (m_app.gameState().isMasterBuilder()) {
                    auto* click = m_app.assets().loadSound("assets/audio/snds/HORN.WAV");
                    if (click) m_app.audio().playSound(click);
                    m_app.scenes().pushDeferred(std::make_unique<ActivityScene>(m_app, 68));
                }
            }
        }
    }
}

void TimeCardScene::update(float /*dt*/) {
    int mx = m_app.input().mouseX();
    int my = m_app.input().mouseY();
    SDL_Point pt = {mx, my};

    m_hoveredButton = -1;
    for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i) {
        auto& b = m_buttons[i];
        SDL_Rect r = {b.cx - b.w / 2, b.cy - b.h / 2, b.w, b.h};
        if (SDL_PointInRect(&pt, &r)) {
            m_hoveredButton = i;
            break;
        }
    }

    m_hoveredCell = -1;
    if (m_hoveredButton < 0) {
        for (int i = 0; i < static_cast<int>(m_cells.size()); ++i) {
            auto& c = m_cells[i];
            SDL_Rect r = {c.cx - c.w / 2, c.cy - c.h / 2, c.w, c.h};
            if (SDL_PointInRect(&pt, &r)) {
                m_hoveredCell = i;
                break;
            }
        }
    }
}

void TimeCardScene::render(Renderer& renderer) {
    if (m_background) {
        renderer.drawTextureEx(m_background, 0, 0, 640, 480);
    }

    // Name header
    if (m_nameHeader) {
        int nw, nh;
        SDL_QueryTexture(m_nameHeader, nullptr, nullptr, &nw, &nh);
        renderer.drawTexture(m_nameHeader, m_nameHdrX - nw / 2, m_nameHdrY - nh / 2);

        // Draw player name on the header
        m_app.text().drawTextCentered(
            m_app.gameState().playerName, m_nameHdrY - 8, 640,
            {139, 69, 19, 255}, 20);
    }

    // Time card cells
    for (int i = 0; i < static_cast<int>(m_cells.size()); ++i) {
        auto& cell = m_cells[i];
        bool complete = (cell.moduleId > 0 && m_app.gameState().isAreaComplete(cell.moduleId)) ||
                        (cell.moduleId == 0 && m_app.gameState().isMasterBuilder());

        SDL_Texture* tex = complete ? cell.complete : cell.incomplete;
        if (!tex) tex = cell.complete; // fallback
        if (tex) {
            renderer.drawTexture(tex, cell.cx - cell.w / 2, cell.cy - cell.h / 2);
        }

        if (i == m_hoveredCell) {
            SDL_Rect r = {cell.cx - cell.w / 2, cell.cy - cell.h / 2, cell.w, cell.h};
            SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 40);
            SDL_RenderFillRect(renderer.sdlRenderer(), &r);
        }
    }

    // Buttons
    for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i) {
        auto& btn = m_buttons[i];
        if (btn.texture) {
            renderer.drawTexture(btn.texture, btn.cx - btn.w / 2, btn.cy - btn.h / 2);
        }
        if (i == m_hoveredButton) {
            SDL_Rect r = {btn.cx - btn.w / 2, btn.cy - btn.h / 2, btn.w, btn.h};
            SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 40);
            SDL_RenderFillRect(renderer.sdlRenderer(), &r);
        }
    }
}
