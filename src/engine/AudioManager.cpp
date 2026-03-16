#include "AudioManager.h"
#include <iostream>

AudioManager::~AudioManager() {
    if (m_initialized) {
        Mix_CloseAudio();
    }
}

bool AudioManager::init() {
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << "\n";
        return false;
    }
    Mix_AllocateChannels(16);
    m_initialized = true;
    return true;
}

void AudioManager::playSound(Mix_Chunk* chunk, int loops) {
    if (chunk && m_initialized) {
        Mix_PlayChannel(-1, chunk, loops);
    }
}

void AudioManager::playAmbient(Mix_Chunk* chunk) {
    if (chunk && m_initialized) {
        stopAmbient();
        m_ambientChannel = Mix_PlayChannel(-1, chunk, -1);
    }
}

void AudioManager::stopAmbient() {
    if (m_ambientChannel >= 0 && m_initialized) {
        Mix_HaltChannel(m_ambientChannel);
        m_ambientChannel = -1;
    }
}

void AudioManager::setMasterVolume(int volume) {
    if (m_initialized) {
        Mix_MasterVolume(volume);
    }
}
