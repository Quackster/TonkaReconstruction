#pragma once

#include "engine/SceneManager.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>

class Application;

class GarageScene : public Scene {
public:
    explicit GarageScene(Application& app);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    Application& m_app;
    SDL_Texture* m_background = nullptr;

    // Vehicles on display (clickable to select for painting)
    struct Vehicle {
        SDL_Texture* texture = nullptr;
        SDL_Rect rect;        // screen position
        std::string name;
        int moduleId;         // module to go to when driving this vehicle
    };
    std::vector<Vehicle> m_vehicles;
    int m_selectedVehicle = -1;
    int m_hoveredVehicle = -1;

    // Paint canvas
    SDL_Texture* m_canvas = nullptr;
    SDL_Rect m_canvasRect = {};
    bool m_painting = false;

    // Paint colors (the original game uses spray paint cans)
    struct PaintColor {
        SDL_Color color;
        SDL_Rect rect;
    };
    std::vector<PaintColor> m_paintColors;
    int m_selectedColor = 0;

    // Paint strokes stored as colored dots
    struct PaintStroke {
        int x, y;
        SDL_Color color;
        int radius;
    };
    std::vector<PaintStroke> m_strokes;

    SDL_Texture* m_paintSurface = nullptr;

    void setupVehicles();
    void setupPaintColors();
    void applyPaint(int x, int y);
};
