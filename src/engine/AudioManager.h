#pragma once

#include <SDL2/SDL_mixer.h>
#include <string>

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    bool init();
    void playSound(Mix_Chunk* chunk, int loops = 0);
    void playAmbient(Mix_Chunk* chunk);
    void stopAmbient();
    void setMasterVolume(int volume); // 0-128

private:
    bool m_initialized = false;
    int m_ambientChannel = -1;
};
