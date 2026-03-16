#include "QuarryScene.h"
#include "engine/Application.h"
#include "game/ActivityScene.h"
#include <cstdio>
#include <algorithm>
#include <cmath>

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
    m_objectsCleared = 0;
    m_goldFound = false;
    m_areaComplete = false;
    m_completionTimer = 0.0f;
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

    // Static scenery
    struct SceneryDef { int idx; int cx; int cy; };
    SceneryDef defs[] = {
        {2,   72, 144},
        {18,  46, 300},
        {19, 231, 300},
    };
    for (auto& sd : defs) {
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

    // Front Loader (overlay 0, 96 frames)
    // Animation phases based on frame analysis:
    //   0-15:  top-down approach (driving toward camera)
    //   16-35: turning/positioning
    //   36-55: digging/scooping with bucket
    //   56-75: carrying loaded bucket
    //   76-95: dumping at hopper
    m_frontLoader = std::make_unique<Vehicle>();
    m_frontLoader->loadFrames(m_app.assets(), "assets/images/module01/overlay_000", 96);
    m_frontLoader->setPosition(300, 350);
    m_frontLoader->setSpeed(120.0f);
    m_frontLoader->setFrameRate(10.0f);
    m_frontLoader->setLooping(false);
    setLoaderAction(LoaderAction::Idle);

    // Crane (overlay 3, 12 frames)
    m_crane = std::make_unique<Vehicle>();
    m_crane->loadFrames(m_app.assets(), "assets/images/module01/overlay_003", 12);
    m_crane->setPosition(900, 250);
    m_crane->setFrameRate(4.0f);
    m_crane->setLooping(true);

    // Bulldozer (overlay 4, 3 frames)
    m_bulldozer = std::make_unique<Vehicle>();
    m_bulldozer->loadFrames(m_app.assets(), "assets/images/module01/overlay_004", 3);
    m_bulldozer->setPosition(550, 380);
    m_bulldozer->setFrameRate(3.0f);
    m_bulldozer->setLooping(true);

    // Draggable rock/boulder objects
    struct ObjDef { int idx; float x; float y; int nFrames; const char* name; };
    ObjDef objDefs[] = {
        {5,  613, 320, 3, "Red Pile"},
        {6,  844, 320, 3, "Black Pile"},
        {7,  150, 250, 4, "Boulder A"},
        {8,  420, 250, 4, "Boulder B"},
        {9,  680, 250, 4, "Boulder C"},
        {10, 950, 250, 4, "Boulder D"},
        {11,1100, 300, 5, "Hopper"},
    };
    for (auto& od : objDefs) {
        DraggableObject dobj;
        dobj.name = od.name;
        char buf[256];
        std::snprintf(buf, sizeof(buf), "assets/images/module01/overlay_%03d", od.idx);
        std::string base = buf;
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

void QuarryScene::setLoaderAction(LoaderAction action) {
    m_loaderAction = action;
    switch (action) {
        case LoaderAction::Idle:
            m_loaderPhaseStart = 0;
            m_loaderPhaseEnd = 0;
            m_frontLoader->stop();
            m_frontLoader->setFrame(0);
            break;
        case LoaderAction::Approach:
            m_loaderPhaseStart = 0;
            m_loaderPhaseEnd = 15;
            m_frontLoader->setFrame(0);
            m_frontLoader->play();
            break;
        case LoaderAction::Dig:
            m_loaderPhaseStart = 36;
            m_loaderPhaseEnd = 55;
            m_frontLoader->setFrame(36);
            m_frontLoader->play();
            break;
        case LoaderAction::Scoop:
            m_loaderPhaseStart = 36;
            m_loaderPhaseEnd = 55;
            m_frontLoader->setFrame(36);
            m_frontLoader->play();
            break;
        case LoaderAction::Carry:
            m_loaderPhaseStart = 56;
            m_loaderPhaseEnd = 75;
            m_frontLoader->setFrame(56);
            m_frontLoader->play();
            break;
        case LoaderAction::Dump:
            m_loaderPhaseStart = 76;
            m_loaderPhaseEnd = 95;
            m_frontLoader->setFrame(76);
            m_frontLoader->play();
            break;
    }
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
        // Crane drag mode: pick up small objects
        if (m_activeVehicle == m_crane.get()) {
            for (int i = 0; i < static_cast<int>(m_objects.size()); ++i) {
                auto& dobj = m_objects[i];
                if (dobj.cleared) continue;
                if (dobj.obj.hitTest(mx, my)) {
                    m_draggedObject = i;
                    dobj.dragging = true;
                    dobj.dragOffsetX = dobj.obj.x() - mx;
                    dobj.dragOffsetY = dobj.obj.y() - my;
                    auto* sfx = m_app.assets().loadSound("assets/audio/snds/CRANE1.WAV");
                    if (sfx) m_app.audio().playSound(sfx);
                    return;
                }
            }
        }

        if (m_draggedObject < 0) {
            handleClick(mx, my);
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_draggedObject >= 0) {
            auto& dobj = m_objects[m_draggedObject];
            dobj.dragging = false;

            // Check if dropped near the hopper (last object in list)
            if (m_objects.size() > 1) {
                auto& hopper = m_objects.back();
                float dx = dobj.obj.x() - hopper.obj.x();
                float dy = dobj.obj.y() - hopper.obj.y();
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < 80.0f && m_draggedObject != static_cast<int>(m_objects.size()) - 1) {
                    dobj.cleared = true;
                    dobj.obj.setVisible(false);
                    m_objectsCleared++;
                    auto* sfx = m_app.assets().loadSound("assets/audio/snds/THUDMTL.WAV");
                    if (sfx) m_app.audio().playSound(sfx);
                    checkCompletion();
                } else {
                    auto* sfx = m_app.assets().loadSound("assets/audio/snds/THUDSTON.WAV");
                    if (sfx) m_app.audio().playSound(sfx);
                }
            }
            m_draggedObject = -1;
        }
    }

    if (event.type == SDL_MOUSEMOTION && m_draggedObject >= 0) {
        auto& dobj = m_objects[m_draggedObject];
        float newX = mx + dobj.dragOffsetX;
        float newY = my + dobj.dragOffsetY;
        newX = std::max(0.0f, std::min(static_cast<float>(m_bgWidth), newX));
        newY = std::max(0.0f, std::min(480.0f, newY));
        dobj.obj.setPosition(newX, newY);
    }
}

void QuarryScene::handleClick(int worldX, int worldY) {
    // Select vehicles
    auto trySelect = [&](Vehicle* v, const char* /*name*/) -> bool {
        if (!v || !v->visible() || v->frameCount() == 0) return false;
        if (v->hitTest(worldX, worldY)) {
            if (m_activeVehicle == v) {
                m_activeVehicle = nullptr;
                v->stop();
            } else {
                if (m_activeVehicle) m_activeVehicle->stop();
                m_activeVehicle = v;
                if (v == m_crane.get()) {
                    v->play(); // Crane animates while selected
                }
                auto* sfx = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
                if (sfx) m_app.audio().playSound(sfx);
            }
            return true;
        }
        return false;
    };

    if (trySelect(m_frontLoader.get(), "Front Loader")) return;
    if (trySelect(m_crane.get(), "Crane")) return;
    if (trySelect(m_bulldozer.get(), "Bulldozer")) return;

    // Click on objects with front loader to dig/scoop
    if (m_activeVehicle == m_frontLoader.get()) {
        for (auto& dobj : m_objects) {
            if (dobj.cleared) continue;
            if (dobj.obj.hitTest(worldX, worldY)) {
                // Drive front loader toward the object
                m_frontLoader->setPath({
                    {m_frontLoader->x(), m_frontLoader->y()},
                    {dobj.obj.x() - 60, dobj.obj.y() + 20}
                });
                m_frontLoader->startDriving();
                setLoaderAction(LoaderAction::Approach);
                m_frontLoader->setOnArrived([this]() {
                    setLoaderAction(LoaderAction::Dig);
                    auto* sfx = m_app.assets().loadSound("assets/audio/snds/BLADE.WAV");
                    if (sfx) m_app.audio().playSound(sfx);
                });
                auto* sfx = m_app.assets().loadSound("assets/audio/snds/DRIVEON1.WAV");
                if (sfx) m_app.audio().playSound(sfx);
                return;
            }
        }

        // Click empty ground: drive there
        m_frontLoader->setPath({
            {m_frontLoader->x(), m_frontLoader->y()},
            {static_cast<float>(worldX), static_cast<float>(worldY)}
        });
        m_frontLoader->startDriving();
        setLoaderAction(LoaderAction::Approach);
        m_frontLoader->setOnArrived([this]() {
            setLoaderAction(LoaderAction::Idle);
        });
        auto* sfx = m_app.assets().loadSound("assets/audio/snds/DRIVEON1.WAV");
        if (sfx) m_app.audio().playSound(sfx);
        return;
    }

    // Click empty ground with bulldozer: push toward click
    if (m_activeVehicle == m_bulldozer.get()) {
        m_bulldozer->setPath({
            {m_bulldozer->x(), m_bulldozer->y()},
            {static_cast<float>(worldX), static_cast<float>(worldY)}
        });
        m_bulldozer->startDriving();
        m_bulldozer->play();
        m_bulldozer->setOnArrived([this]() {
            m_bulldozer->stop();
            m_bulldozer->setActive(true);
        });
        auto* sfx = m_app.assets().loadSound("assets/audio/snds/DRIVEON1.WAV");
        if (sfx) m_app.audio().playSound(sfx);
    }
}

void QuarryScene::checkCompletion() {
    // Per TONKA.TXT: clear red/black piles and find gold for certificate
    // Simplified: clear at least 3 objects
    if (m_objectsCleared >= 3 && !m_areaComplete) {
        m_areaComplete = true;
        m_completionTimer = 3.0f;
        m_app.gameState().completeArea(1);
        m_app.gameState().save("tonka_save.dat");

        auto* sfx = m_app.assets().loadSound("assets/audio/snds/HORN.WAV");
        if (sfx) m_app.audio().playSound(sfx);
    }
}

void QuarryScene::update(float dt) {
    updateScroll();

    if (m_frontLoader) {
        m_frontLoader->update(dt);
        // Clamp animation to current phase range
        if (m_frontLoader->isPlaying() && m_loaderPhaseEnd > m_loaderPhaseStart) {
            if (m_frontLoader->currentFrame() > m_loaderPhaseEnd) {
                m_frontLoader->setFrame(m_loaderPhaseEnd);
                m_frontLoader->stop();
            }
        }
    }
    if (m_crane) m_crane->update(dt);
    if (m_bulldozer) m_bulldozer->update(dt);

    for (auto& dobj : m_objects) {
        dobj.obj.update(dt);
    }

    // Completion timer
    if (m_areaComplete && m_completionTimer > 0.0f) {
        m_completionTimer -= dt;
        if (m_completionTimer <= 0.0f) {
            // Show quarry certificate (MODULE61)
            m_app.scenes().pushDeferred(std::make_unique<ActivityScene>(m_app, 61));
        }
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

    // Scenery (behind vehicles)
    for (auto& s : m_scenery) {
        int dx = s.cx - s.w / 2 - m_scrollX;
        int dy = s.cy - s.h / 2;
        if (dx + s.w < 0 || dx > 640) continue;
        renderer.drawTexture(s.texture, dx, dy);
    }

    // Objects (rocks, piles)
    for (auto& dobj : m_objects) {
        dobj.obj.render(renderer, m_scrollX);
    }

    // Vehicles
    if (m_bulldozer) m_bulldozer->render(renderer, m_scrollX);
    if (m_frontLoader) m_frontLoader->render(renderer, m_scrollX);
    if (m_crane) m_crane->render(renderer, m_scrollX);

    // Active vehicle highlight
    if (m_activeVehicle && m_activeVehicle->visible()) {
        SDL_Rect wr = m_activeVehicle->worldRect();
        SDL_Rect sr = {wr.x - m_scrollX, wr.y, wr.w, wr.h};
        if (sr.x + sr.w >= 0 && sr.x <= 640) {
            SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 30);
            SDL_RenderFillRect(renderer.sdlRenderer(), &sr);
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 180);
            SDL_RenderDrawRect(renderer.sdlRenderer(), &sr);
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
        m_app.text().drawTextCentered("Quarry Complete!", 220, 640, {255, 204, 0, 255}, 28);
        m_app.text().drawTextCentered("Certificate incoming...", 250, 640, {255, 255, 255, 255}, 16);
    }
}
