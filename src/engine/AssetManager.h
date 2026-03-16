#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>

class AssetManager {
public:
    explicit AssetManager(SDL_Renderer* renderer);
    ~AssetManager();

    SDL_Texture* loadTexture(const std::string& path);
    Mix_Chunk* loadSound(const std::string& path);

    SDL_Texture* getTexture(const std::string& path);
    Mix_Chunk* getSound(const std::string& path);

    void unloadAll();

private:
    SDL_Renderer* m_renderer;
    std::unordered_map<std::string, SDL_Texture*> m_textures;
    std::unordered_map<std::string, Mix_Chunk*> m_sounds;
};
