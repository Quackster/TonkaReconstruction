#pragma once

#include <SDL2/SDL.h>
#include <string>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init(SDL_Window* window, int logicalW, int logicalH);
    void clear();
    void present();

    void drawTexture(SDL_Texture* tex, int x, int y);
    void drawTexture(SDL_Texture* tex, const SDL_Rect* src, const SDL_Rect* dst);
    void drawTextureEx(SDL_Texture* tex, int x, int y, int w, int h);

    SDL_Renderer* sdlRenderer() { return m_renderer; }

    // Convert window coords to logical (game) coords
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;

    int logicalWidth() const { return m_logicalW; }
    int logicalHeight() const { return m_logicalH; }

private:
    SDL_Renderer* m_renderer = nullptr;
    int m_logicalW = 640;
    int m_logicalH = 480;
};
