#include "InputManager.h"
#include "Renderer.h"

void InputManager::handleEvent(const SDL_Event& event, const Renderer& renderer) {
    switch (event.type) {
        case SDL_MOUSEMOTION:
            const_cast<Renderer&>(renderer).windowToLogical(
                event.motion.x, event.motion.y, m_mouseX, m_mouseY);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                m_mouseDown = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                m_mouseDown = false;
            }
            break;
    }
}

void InputManager::update() {
    m_mouseClicked = (!m_mouseDown && m_prevMouseDown);
    m_prevMouseDown = m_mouseDown;
}
