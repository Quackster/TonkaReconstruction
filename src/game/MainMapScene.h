#pragma once

#include "engine/SceneManager.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>

class Application;

struct MapOverlay {
    SDL_Texture* texture = nullptr;
    int cx, cy;    // center position on screen
    int w, h;      // sprite dimensions
    int type;      // 0=area image, 1=nav element, 2=menu, 3=time card
    int moduleId;  // destination module (for type 1)
};

class MainMapScene : public Scene {
public:
    explicit MainMapScene(Application& app);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    Application& m_app;
    SDL_Texture* m_background = nullptr;
    std::vector<MapOverlay> m_overlays;
    int m_hoveredNav = -1;

    void loadOverlays();
    void navigateTo(int moduleId);
};
