#include "AssetManager.h"
#include <SDL2/SDL_image.h>
#include <iostream>

AssetManager::AssetManager(SDL_Renderer* renderer)
    : m_renderer(renderer) {}

AssetManager::~AssetManager() {
    unloadAll();
}

SDL_Texture* AssetManager::loadTexture(const std::string& path) {
    auto it = m_textures.find(path);
    if (it != m_textures.end()) {
        return it->second;
    }

    SDL_Texture* tex = IMG_LoadTexture(m_renderer, path.c_str());
    if (!tex) {
        // Silently return null for missing textures - not all overlays are extracted
        return nullptr;
    }

    m_textures[path] = tex;
    return tex;
}

Mix_Chunk* AssetManager::loadSound(const std::string& path) {
    auto it = m_sounds.find(path);
    if (it != m_sounds.end()) {
        return it->second;
    }

    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::cerr << "Failed to load sound: " << path << " - " << Mix_GetError() << "\n";
        return nullptr;
    }

    m_sounds[path] = chunk;
    return chunk;
}

SDL_Texture* AssetManager::getTexture(const std::string& path) {
    auto it = m_textures.find(path);
    return (it != m_textures.end()) ? it->second : nullptr;
}

Mix_Chunk* AssetManager::getSound(const std::string& path) {
    auto it = m_sounds.find(path);
    return (it != m_sounds.end()) ? it->second : nullptr;
}

void AssetManager::unloadAll() {
    for (auto& [_, tex] : m_textures) {
        SDL_DestroyTexture(tex);
    }
    m_textures.clear();

    for (auto& [_, chunk] : m_sounds) {
        Mix_FreeChunk(chunk);
    }
    m_sounds.clear();
}
