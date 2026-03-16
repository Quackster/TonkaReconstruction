#pragma once

#include "engine/SceneManager.h"
#include <SDL2/SDL.h>
#include <vector>

class Application;

// Time Card / Options screen (MODULE13).
// Shows player progress across all areas with completion stamps.
// Clicking a completed area's stamp shows its certificate.
// QUIT button exits the game, CANCEL returns to map.
class TimeCardScene : public Scene {
public:
    explicit TimeCardScene(Application& app);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    Application& m_app;
    SDL_Texture* m_background = nullptr;

    // Time card cells (area completion stamps)
    struct Cell {
        SDL_Texture* complete = nullptr;   // frame 0: completed
        SDL_Texture* incomplete = nullptr; // frame 1: empty
        int cx, cy, w, h;
        int moduleId;                      // which area this cell represents
    };
    std::vector<Cell> m_cells;

    // Buttons
    struct Button {
        SDL_Texture* texture = nullptr;
        int cx, cy, w, h;
        enum Action { Quit, Cancel, None } action = None;
    };
    std::vector<Button> m_buttons;

    // Name header
    SDL_Texture* m_nameHeader = nullptr;
    int m_nameHdrX = 0, m_nameHdrY = 0;

    int m_hoveredCell = -1;
    int m_hoveredButton = -1;

    void loadAssets();
};
