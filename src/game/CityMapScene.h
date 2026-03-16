#pragma once

#include "engine/SceneManager.h"
#include <SDL2/SDL.h>
#include <vector>

class Application;

// City grid hub (MODULE11) - sub-navigation to Park (4), Castle (5), Hi-Rise (6).
// Shows 3 construction sites on an isometric city grid. Each site displays
// the building that was constructed there. Click a site to enter that area.
class CityMapScene : public Scene {
public:
    explicit CityMapScene(Application& app);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    Application& m_app;
    SDL_Texture* m_background = nullptr;

    // 3 construction sites, each with overlays for each area + empty state
    struct Site {
        int moduleId;              // which area (4=Park, 5=Castle, 6=HiRise)
        SDL_Texture* texture = nullptr;
        int cx, cy, w, h;
    };
    std::vector<Site> m_sites;
    int m_hoveredSite = -1;

    void loadSites();
};
