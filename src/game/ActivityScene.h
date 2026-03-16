#pragma once

#include "engine/SceneManager.h"
#include "data/ModuleData.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

class Application;

// Data-driven interactive scene for all game areas.
// Loads background + overlays from MODULE data, handles scrolling, animation,
// click-to-activate overlays, vehicle arrivals, and area completion.
class ActivityScene : public Scene {
public:
    ActivityScene(Application& app, int moduleId);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

protected:
    Application& m_app;
    int m_moduleId;
    ModuleInfo m_modInfo;

    SDL_Texture* m_background = nullptr;
    int m_bgWidth = 640;
    int m_scrollX = 0;
    int m_maxScrollX = 0;

    struct SceneOverlay {
        SDL_Texture* texture = nullptr;
        std::vector<SDL_Texture*> frames;
        int cx = 0, cy = 0;
        int w = 0, h = 0;
        int type = 0;         // 0=scenery, 1=interactive, 2=menu, 3=UI
        int navModule = 0;
        int entryIndex = -1;  // original entry index in module
        int currentFrame = 0;
        float frameTimer = 0.0f;
        bool animated = false;
        bool visible = true;
        bool activated = false;  // has the player interacted with this?
        bool clickable = true;
    };
    std::vector<SceneOverlay> m_overlays;
    int m_hoveredOverlay = -1;
    int m_activeOverlay = -1;  // currently selected interactive overlay

    // Draggable objects (type 0 small overlays that can be picked up)
    int m_draggedOverlay = -1;
    float m_dragOffX = 0, m_dragOffY = 0;

    // Area completion tracking
    int m_activatedCount = 0;
    int m_requiredActivations = 0;
    bool m_areaComplete = false;
    float m_completionTimer = 0.0f;

    std::string moduleName() const;
    void loadOverlays();
    void updateScroll(float dt);
    int helpModuleId() const;
    void checkCompletion();
    void showCertificate();
    bool isInteractive(const SceneOverlay& ov) const;
    bool isDraggable(const SceneOverlay& ov) const;
};
