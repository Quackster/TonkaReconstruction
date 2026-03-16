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

    // Static scenery
    struct SceneryItem {
        SDL_Texture* texture = nullptr;
        int cx, cy, w, h;
    };
    std::vector<SceneryItem> m_scenery;

    // Vehicles
    std::unique_ptr<Vehicle> m_frontLoader;
    std::unique_ptr<Vehicle> m_crane;
    std::unique_ptr<Vehicle> m_bulldozer;

    // Front loader animation phases (96 frames segmented)
    enum class LoaderAction { Idle, Approach, Dig, Scoop, Carry, Dump };
    LoaderAction m_loaderAction = LoaderAction::Idle;
    int m_loaderPhaseStart = 0;
    int m_loaderPhaseEnd = 0;

    // Draggable objects
    struct DraggableObject {
        GameObject obj;
        bool dragging = false;
        float dragOffsetX = 0, dragOffsetY = 0;
        std::string name;
        bool cleared = false;
    };
    std::vector<DraggableObject> m_objects;
    int m_draggedObject = -1;

    Vehicle* m_activeVehicle = nullptr;

    // Completion state
    int m_objectsCleared = 0;
    bool m_goldFound = false;
    bool m_areaComplete = false;
    float m_completionTimer = 0.0f;

    void loadAssets();
    void updateScroll();
    void handleClick(int worldX, int worldY);
    void setLoaderAction(LoaderAction action);
    void checkCompletion();
};
