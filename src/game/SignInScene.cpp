#include "SignInScene.h"
#include "engine/Application.h"
#include "game/MainMapScene.h"
#include <algorithm>

SignInScene::SignInScene(Application& app)
    : m_app(app) {}

void SignInScene::onEnter() {
    m_background = m_app.assets().loadTexture("assets/images/module15/background.png");
    m_timeCard = m_app.assets().loadTexture("assets/images/module15/overlay_000.png");
    m_okButton = m_app.assets().loadTexture("assets/images/module15/overlay_004.png");

    m_playerName = "";
    m_nameReady = false;

    // Try to load existing save
    m_app.gameState().load("tonka_save.dat");
    if (!m_app.gameState().playerName.empty()) {
        m_playerName = m_app.gameState().playerName;
    }

    SDL_StartTextInput();
}

void SignInScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        // Append typed characters (limit to ~15 chars for the name box)
        if (m_playerName.size() < 15) {
            m_playerName += event.text.text;
        }
    }

    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_BACKSPACE && !m_playerName.empty()) {
            m_playerName.pop_back();
        }
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
            if (!m_playerName.empty()) {
                m_nameReady = true;
            }
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (m_okHovered && !m_playerName.empty()) {
            m_nameReady = true;
            auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
            if (click) m_app.audio().playSound(click);
        }
    }
}

void SignInScene::update(float dt) {
    m_cursorBlink += dt;

    int mx = m_app.input().mouseX();
    int my = m_app.input().mouseY();
    SDL_Point pt = {mx, my};
    m_okHovered = SDL_PointInRect(&pt, &m_okRect) == SDL_TRUE;

    if (m_nameReady) {
        m_app.gameState().playerName = m_playerName;
        m_app.gameState().save("tonka_save.dat");
        SDL_StopTextInput();
        m_app.scenes().replaceDeferred(std::make_unique<MainMapScene>(m_app));
    }
}

void SignInScene::render(Renderer& renderer) {
    // Background (construction trailer)
    if (m_background) {
        renderer.drawTextureEx(m_background, 0, 0, 640, 480);
    }

    // Time card overlay
    if (m_timeCard) {
        renderer.drawTextureEx(m_timeCard, m_cardRect.x, m_cardRect.y,
                              m_cardRect.w, m_cardRect.h);
    }

    // Player name text in the red-bordered box
    {
        SDL_Color brown = {139, 69, 19, 255};
        if (!m_playerName.empty()) {
            m_app.text().drawText(m_playerName, m_nameRect.x + 6, m_nameRect.y + 4, brown, 18);
        }

        // Blinking cursor
        if (static_cast<int>(m_cursorBlink * 2) % 2 == 0) {
            SDL_Texture* nameTex = m_playerName.empty() ? nullptr :
                m_app.text().renderText(m_playerName, brown, 18);
            int textW = 0;
            if (nameTex) SDL_QueryTexture(nameTex, nullptr, nullptr, &textW, nullptr);
            int cursorX = m_nameRect.x + 6 + textW;
            SDL_Rect cursorRect = {cursorX, m_nameRect.y + 4, 2, 20};
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 139, 69, 19, 255);
            SDL_RenderFillRect(renderer.sdlRenderer(), &cursorRect);
        }

        // Prompt text
        if (m_playerName.empty()) {
            SDL_Color gray = {160, 140, 100, 180};
            m_app.text().drawText("Type your name", m_nameRect.x + 6, m_nameRect.y + 4, gray, 16);
        }
    }

    // OK button
    if (m_okButton) {
        renderer.drawTextureEx(m_okButton, m_okRect.x, m_okRect.y, m_okRect.w, m_okRect.h);
        if (m_okHovered) {
            SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 40);
            SDL_RenderFillRect(renderer.sdlRenderer(), &m_okRect);
        }
    }
}
