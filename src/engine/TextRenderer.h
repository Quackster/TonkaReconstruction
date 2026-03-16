#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>

class TextRenderer {
public:
    TextRenderer() = default;
    ~TextRenderer();

    bool init(SDL_Renderer* renderer);

    // Render text, returns texture (cached). Caller must NOT free it.
    SDL_Texture* renderText(const std::string& text, SDL_Color color, int fontSize = 16);

    // Draw text at position
    void drawText(const std::string& text, int x, int y,
                  SDL_Color color = {255, 255, 255, 255}, int fontSize = 16);

    // Draw text centered horizontally at y
    void drawTextCentered(const std::string& text, int y, int areaWidth = 640,
                          SDL_Color color = {255, 255, 255, 255}, int fontSize = 16);

private:
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<int, TTF_Font*> m_fonts; // size -> font
    std::unordered_map<std::string, SDL_Texture*> m_cache;

    TTF_Font* getFont(int size);
    std::string cacheKey(const std::string& text, SDL_Color color, int size) const;
};
