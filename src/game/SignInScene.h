#pragma once

#include "engine/SceneManager.h"
#include <SDL2/SDL.h>
#include <string>

class Application;

class SignInScene : public Scene {
public:
    explicit SignInScene(Application& app);

    void onEnter() override;
    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    Application& m_app;
    SDL_Texture* m_background = nullptr;
    SDL_Texture* m_timeCard = nullptr;
    SDL_Texture* m_okButton = nullptr;

    std::string m_playerName;
    bool m_nameReady = false;

    // Time card overlay position (centered)
    SDL_Rect m_cardRect = {80, 20, 480, 420};
    // Name input area on the card (the red-bordered box at top)
    SDL_Rect m_nameRect = {230, 52, 200, 28};
    // OK button position
    SDL_Rect m_okRect = {530, 400, 68, 64};

    bool m_okHovered = false;
    float m_cursorBlink = 0.0f;
};
