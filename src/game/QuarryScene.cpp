#include "QuarryScene.h"
#include "engine/Application.h"
#include "game/ActivityScene.h"
#include <cstdio>
#include <algorithm>
#include <iostream>

QuarryScene::QuarryScene(Application& app)
    : m_app(app) {}

void QuarryScene::onEnter() {
    m_app.gameState().startArea(1);
    loadAssets();

    auto* ambient = m_app.assets().loadSound("assets/audio/ambient/AMBNT1.WAV");
    if (ambient) {
        m_app.audio().playAmbient(ambient);
    }
    m_scrollX = 0;
}

void QuarryScene::onExit() {
    m_app.audio().stopAmbient();
}

void QuarryScene::loadAssets() {
    m_background = m_app.assets().loadTexture("assets/images/module01/background.png");
    if (m_background) {
        SDL_QueryTexture(m_background, nullptr, nullptr, &m_bgWidth, nullptr);
        m_maxScrollX = m_bgWidth - 640;
        if (m_maxScrollX < 0) m_maxScrollX = 0;
    }

    // Load static scenery (type 0 overlays from MODULE01 at known positions)
    struct SceneryDef { int idx; int cx; int cy; };
    SceneryDef sceneryDefs[] = {
        {2,   72, 144},   // Dirt patch
        {18,  46, 300},   // Clipboard
        {19, 231, 300},   // Small object
    };
    for (auto& sd : sceneryDefs) {
        char path[256];
        std::snprintf(path, sizeof(path), "assets/images/module01/overlay_%03d.png", sd.idx);
        auto* tex = m_app.assets().loadTexture(path);
        if (!tex) {
            std::snprintf(path, sizeof(path), "assets/images/module01/overlay_%03d_frame000.png", sd.idx);
            tex = m_app.assets().loadTexture(path);
        }
        if (tex) {
            int w, h;
            SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
            m_scenery.push_back({tex, sd.cx, sd.cy, w, h});
        }
    }

    // Create Front Loader vehicle (overlay 0, 96 animation frames)
    m_frontLoader = std::make_unique<Vehicle>();
    m_frontLoader->loadFrames(m_app.assets(), "assets/images/module01/overlay_000", 96);
    m_frontLoader->setPosition(200, 350);
    m_frontLoader->setSpeed(100.0f);
    m_frontLoader->setFrameRate(12.0f);
    // Define approach path (vehicle drives in from off-screen)
    m_frontLoader->setPath({
        {-100, 400}, {50, 380}, {150, 360}, {200, 350}
    });

    // Create Crane (overlay 3, 12 frames)
    m_crane = std::make_unique<Vehicle>();
    m_crane->loadFrames(m_app.assets(), "assets/images/module01/overlay_003", 12);
    m_crane->setPosition(668, 200);
    m_crane->setFrameRate(6.0f);

    // Create Bulldozer (overlay 4, 3 frames)
    m_bulldozer = std::make_unique<Vehicle>();
    m_bulldozer->loadFrames(m_app.assets(), "assets/images/module01/overlay_004", 3);
    m_bulldozer->setPosition(455, 350);
    m_bulldozer->setFrameRate(4.0f);

    // Draggable rock piles (overlays 5, 6 - rock piles that can be moved)
    struct ObjDef { int idx; float x; float y; int nFrames; const char* name; };
    ObjDef objDefs[] = {
        {5,  613, 300, 3, "Rock Pile"},
        {6,  844, 300, 3, "Rock Pile 2"},
        {7,   71, 200, 4, "Boulder A"},
        {8,  370, 200, 4, "Boulder B"},
        {9,  538, 200, 4, "Boulder C"},
        {10, 785, 200, 4, "Boulder D"},
    };
    for (auto& od : objDefs) {
        DraggableObject dobj;
        dobj.name = od.name;
        std::string base;
        char buf[256];
        std::snprintf(buf, sizeof(buf), "assets/images/module01/overlay_%03d", od.idx);
        base = buf;
        if (od.nFrames > 1) {
            dobj.obj.loadFrames(m_app.assets(), base, od.nFrames);
        } else {
            dobj.obj.loadSingleFrame(m_app.assets(), base + ".png");
        }
        dobj.obj.setPosition(od.x, od.y);
        m_objects.push_back(std::move(dobj));
    }

    m_activeVehicle = nullptr;
}

void QuarryScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT) {
            m_scrollX = std::max(0, m_scrollX - 32);
        } else if (event.key.keysym.sym == SDLK_RIGHT) {
            m_scrollX = std::min(m_maxScrollX, m_scrollX + 32);
        } else if (event.key.keysym.sym == SDLK_h || event.key.keysym.sym == SDLK_F1) {
            m_app.scenes().pushDeferred(std::make_unique<ActivityScene>(m_app, 51));
        }
    }

    int mx = m_app.input().mouseX() + m_scrollX;
    int my = m_app.input().mouseY();

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Check if clicking a draggable object (with crane active)
        if (m_activeVehicle == m_crane.get()) {
            for (int i = 0; i < static_cast<int>(m_objects.size()); ++i) {
                if (m_objects[i].obj.hitTest(mx, my)) {
                    m_draggedObject = i;
                    m_objects[i].dragging = true;
                    m_objects[i].dragOffsetX = m_objects[i].obj.x() - mx;
                    m_objects[i].dragOffsetY = m_objects[i].obj.y() - my;
                    auto* grab = m_app.assets().loadSound("assets/audio/snds/CRANE1.WAV");
                    if (grab) m_app.audio().playSound(grab);
                    break;
                }
            }
        }

        if (m_draggedObject < 0) {
            handleClick(mx, my);
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_draggedObject >= 0) {
            m_objects[m_draggedObject].dragging = false;
            auto* drop = m_app.assets().loadSound("assets/audio/snds/CRANE2.WAV");
            if (drop) m_app.audio().playSound(drop);
            m_draggedObject = -1;
        }
    }

    if (event.type == SDL_MOUSEMOTION && m_draggedObject >= 0) {
        auto& dobj = m_objects[m_draggedObject];
        float newX = mx + dobj.dragOffsetX;
        float newY = my + dobj.dragOffsetY;
        // Clamp to world bounds
        newX = std::max(0.0f, std::min(static_cast<float>(m_bgWidth), newX));
        newY = std::max(0.0f, std::min(480.0f, newY));
        dobj.obj.setPosition(newX, newY);
    }
}

void QuarryScene::handleClick(int worldX, int worldY) {
    // Check vehicle clicks
    auto trySelect = [&](Vehicle* v, const char* name) -> bool {
        if (!v || !v->visible() || v->frameCount() == 0) return false;
        if (v->hitTest(worldX, worldY)) {
            if (m_activeVehicle == v) {
                // Deselect
                m_activeVehicle = nullptr;
                v->stop();
                std::cout << "Deselected " << name << "\n";
            } else {
                // Select this vehicle
                if (m_activeVehicle) m_activeVehicle->stop();
                m_activeVehicle = v;
                v->play();
                std::cout << "Selected " << name << "\n";
                auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
                if (click) m_app.audio().playSound(click);
            }
            return true;
        }
        return false;
    };

    if (trySelect(m_frontLoader.get(), "Front Loader")) return;
    if (trySelect(m_crane.get(), "Crane")) return;
    if (trySelect(m_bulldozer.get(), "Bulldozer")) return;

    // If a vehicle is active and we clicked empty space, move it there
    if (m_activeVehicle && m_activeVehicle != m_crane.get()) {
        // Simple direct movement for now
        m_activeVehicle->setPath({
            {m_activeVehicle->x(), m_activeVehicle->y()},
            {static_cast<float>(worldX), static_cast<float>(worldY)}
        });
        m_activeVehicle->startDriving();
        m_activeVehicle->setOnArrived([this]() {
            if (m_activeVehicle) {
                m_activeVehicle->setActive(true);
            }
        });
        auto* drive = m_app.assets().loadSound("assets/audio/snds/DRIVEON1.WAV");
        if (drive) m_app.audio().playSound(drive);
    }
}

void QuarryScene::update(float dt) {
    updateScroll();

    if (m_frontLoader) m_frontLoader->update(dt);
    if (m_crane) m_crane->update(dt);
    if (m_bulldozer) m_bulldozer->update(dt);

    for (auto& dobj : m_objects) {
        dobj.obj.update(dt);
    }
}

void QuarryScene::updateScroll() {
    int mx = m_app.input().mouseX();
    const int edgeZone = 40;
    const int scrollSpeed = 4;

    if (mx < edgeZone && m_scrollX > 0) {
        m_scrollX = std::max(0, m_scrollX - scrollSpeed);
    } else if (mx > 640 - edgeZone && m_scrollX < m_maxScrollX) {
        m_scrollX = std::min(m_maxScrollX, m_scrollX + scrollSpeed);
    }
}

void QuarryScene::render(Renderer& renderer) {
    // Scrolled background
    if (m_background) {
        SDL_Rect src = {m_scrollX, 0, 640, 480};
        SDL_Rect dst = {0, 0, 640, 480};
        renderer.drawTexture(m_background, &src, &dst);
    }

    // Scenery
    for (auto& s : m_scenery) {
        int dx = s.cx - s.w / 2 - m_scrollX;
        int dy = s.cy - s.h / 2;
        if (dx + s.w < 0 || dx > 640) continue;
        renderer.drawTexture(s.texture, dx, dy);
    }

    // Draggable objects
    for (auto& dobj : m_objects) {
        dobj.obj.render(renderer, m_scrollX);
    }

    // Vehicles (rendered on top)
    if (m_bulldozer) m_bulldozer->render(renderer, m_scrollX);
    if (m_frontLoader) m_frontLoader->render(renderer, m_scrollX);
    if (m_crane) m_crane->render(renderer, m_scrollX);

    // Active vehicle highlight
    auto highlightVehicle = [&](Vehicle* v) {
        if (!v || !v->visible() || v != m_activeVehicle) return;
        SDL_Rect wr = v->worldRect();
        SDL_Rect sr = {wr.x - m_scrollX, wr.y, wr.w, wr.h};
        if (sr.x + sr.w < 0 || sr.x > 640) return;
        SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 30);
        SDL_RenderFillRect(renderer.sdlRenderer(), &sr);
        SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 180);
        SDL_RenderDrawRect(renderer.sdlRenderer(), &sr);
    };

    highlightVehicle(m_frontLoader.get());
    highlightVehicle(m_crane.get());
    highlightVehicle(m_bulldozer.get());
}
