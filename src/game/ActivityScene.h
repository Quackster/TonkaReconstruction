#pragma once

#include "engine/SceneManager.h"
#include "data/ModuleData.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

class Application;

// Scene that renders any module's background + positioned overlays.
// Used for all activity areas (Quarry, Avalanche, Desert, City areas, etc.)
class ActivityScene : public Scene {
public:
    ActivityScene(Application& app, int moduleId);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    Application& m_app;
    int m_moduleId;
    ModuleInfo m_modInfo;

    SDL_Texture* m_background = nullptr;
    int m_bgWidth = 640;
    int m_scrollX = 0;
    int m_maxScrollX = 0;

    struct SceneOverlay {
        SDL_Texture* texture = nullptr;
        std::vector<SDL_Texture*> frames; // for animated overlays
        int cx = 0, cy = 0;
        int w = 0, h = 0;
        int type = 0;
        int navModule = 0;
        int currentFrame = 0;
        float frameTimer = 0.0f;
        bool animated = false;
        bool visible = true;
    };
    std::vector<SceneOverlay> m_overlays;
    int m_hoveredOverlay = -1;

    std::string moduleName() const;
    void loadOverlays();
    void updateScroll(float dt);
    int helpModuleId() const;
};
