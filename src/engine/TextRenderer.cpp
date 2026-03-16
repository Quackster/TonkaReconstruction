#include "TextRenderer.h"
#include <cstdio>

TextRenderer::~TextRenderer() {
    for (auto& [_, tex] : m_cache) {
        SDL_DestroyTexture(tex);
    }
    for (auto& [_, font] : m_fonts) {
        TTF_CloseFont(font);
    }
    m_cache.clear();
    m_fonts.clear();
}

bool TextRenderer::init(SDL_Renderer* renderer) {
    m_renderer = renderer;
    if (TTF_Init() < 0) {
        return false;
    }
    return true;
}

TTF_Font* TextRenderer::getFont(int size) {
    auto it = m_fonts.find(size);
    if (it != m_fonts.end()) return it->second;

    // Try common system font paths
    const char* fontPaths[] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        nullptr
    };

    TTF_Font* font = nullptr;
    for (int i = 0; fontPaths[i]; ++i) {
        font = TTF_OpenFont(fontPaths[i], size);
        if (font) break;
    }

    if (font) {
        m_fonts[size] = font;
    }
    return font;
}

std::string TextRenderer::cacheKey(const std::string& text, SDL_Color color, int size) const {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "_%d_%d_%d_%d_%d", color.r, color.g, color.b, color.a, size);
    return text + buf;
}

SDL_Texture* TextRenderer::renderText(const std::string& text, SDL_Color color, int fontSize) {
    if (text.empty()) return nullptr;

    std::string key = cacheKey(text, color, fontSize);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) return it->second;

    TTF_Font* font = getFont(fontSize);
    if (!font) return nullptr;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return nullptr;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);

    if (tex) {
        m_cache[key] = tex;
    }
    return tex;
}

void TextRenderer::drawText(const std::string& text, int x, int y,
                            SDL_Color color, int fontSize) {
    SDL_Texture* tex = renderText(text, color, fontSize);
    if (!tex) return;

    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
}

void TextRenderer::drawTextCentered(const std::string& text, int y, int areaWidth,
                                    SDL_Color color, int fontSize) {
    SDL_Texture* tex = renderText(text, color, fontSize);
    if (!tex) return;

    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst = {(areaWidth - w) / 2, y, w, h};
    SDL_RenderCopy(m_renderer, tex, nullptr, &dst);
}
