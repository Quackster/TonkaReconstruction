#pragma once

#include "engine/SceneManager.h"
#include "objects/Vehicle.h"
#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include <string>

class Application;

class QuarryScene : public Scene {
public:
    explicit QuarryScene(Application& app);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    Application& m_app;
    SDL_Texture* m_background = nullptr;
    int m_bgWidth = 1280;
    int m_scrollX = 0;
    int m_maxScrollX = 0;

    // Static scenery overlays
    struct SceneryItem {
        SDL_Texture* texture = nullptr;
        int cx, cy, w, h;
    };
    std::vector<SceneryItem> m_scenery;

    // Vehicles
    std::unique_ptr<Vehicle> m_frontLoader;
    std::unique_ptr<Vehicle> m_crane;
    std::unique_ptr<Vehicle> m_bulldozer;

    // Draggable objects (rocks, gold, etc.)
    struct DraggableObject {
        GameObject obj;
        bool dragging = false;
        float dragOffsetX = 0, dragOffsetY = 0;
        std::string name;
    };
    std::vector<DraggableObject> m_objects;
    int m_draggedObject = -1;

    Vehicle* m_activeVehicle = nullptr;

    void loadAssets();
    void updateScroll();
    void handleClick(int worldX, int worldY);
};
