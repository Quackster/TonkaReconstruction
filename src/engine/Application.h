#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <memory>

#include "Renderer.h"
#include "AssetManager.h"
#include "AudioManager.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "TextRenderer.h"
#include "data/GameState.h"

class Application {
public:
    Application() = default;
    ~Application();

    bool init(const std::string& title, int width, int height);
    void run();
    void quit();

    Renderer& renderer() { return *m_renderer; }
    AssetManager& assets() { return *m_assets; }
    AudioManager& audio() { return *m_audio; }
    InputManager& input() { return *m_input; }
    SceneManager& scenes() { return *m_scenes; }
    TextRenderer& text() { return m_text; }
    GameState& gameState() { return m_gameState; }

    int nativeWidth() const { return m_nativeW; }
    int nativeHeight() const { return m_nativeH; }

private:
    void handleEvents();
    void update(float dt);
    void render();

    SDL_Window* m_window = nullptr;
    bool m_running = false;
    int m_nativeW = 640;
    int m_nativeH = 480;

    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<AssetManager> m_assets;
    std::unique_ptr<AudioManager> m_audio;
    std::unique_ptr<InputManager> m_input;
    std::unique_ptr<SceneManager> m_scenes;
    TextRenderer m_text;
    GameState m_gameState;
};
