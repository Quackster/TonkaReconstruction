#include "Renderer.h"
#include <iostream>

Renderer::~Renderer() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
    }
}

bool Renderer::init(SDL_Window* window, int logicalW, int logicalH) {
    m_logicalW = logicalW;
    m_logicalH = logicalH;

    m_renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        return false;
    }

    // Set logical size for automatic scaling with 4:3 aspect ratio
    SDL_RenderSetLogicalSize(m_renderer, logicalW, logicalH);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);

    return true;
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::present() {
    SDL_RenderPresent(m_renderer);
}

void Renderer::drawTexture(SDL_Texture* tex, int x, int y) {
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
}

void Renderer::drawTexture(SDL_Texture* tex, const SDL_Rect* src, const SDL_Rect* dst) {
    SDL_RenderCopy(m_renderer, tex, src, dst);
}

void Renderer::drawTextureEx(SDL_Texture* tex, int x, int y, int w, int h) {
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
}

void Renderer::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    float sx, sy;
    SDL_RenderWindowToLogical(m_renderer, wx, wy, &sx, &sy);
    lx = static_cast<int>(sx);
    ly = static_cast<int>(sy);
}
