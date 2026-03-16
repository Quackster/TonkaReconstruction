#pragma once

#include <SDL2/SDL.h>

class Renderer;

class InputManager {
public:
    void handleEvent(const SDL_Event& event, const Renderer& renderer);
    void update();

    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }
    bool mouseDown() const { return m_mouseDown; }
    bool mouseClicked() const { return m_mouseClicked; }

private:
    int m_mouseX = 0;
    int m_mouseY = 0;
    bool m_mouseDown = false;
    bool m_mouseClicked = false;
    bool m_prevMouseDown = false;
};
