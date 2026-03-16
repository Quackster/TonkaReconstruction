#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>

class Renderer;
class AssetManager;

class GameObject {
public:
    GameObject() = default;
    virtual ~GameObject() = default;

    void setPosition(float x, float y) { m_x = x; m_y = y; }
    float x() const { return m_x; }
    float y() const { return m_y; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    void setVisible(bool v) { m_visible = v; }
    bool visible() const { return m_visible; }

    bool loadFrames(AssetManager& assets, const std::string& basePath, int count);
    bool loadSingleFrame(AssetManager& assets, const std::string& path);

    void setFrameRate(float fps) { m_frameRate = fps; }
    void play() { m_playing = true; }
    void stop() { m_playing = false; }
    void setFrame(int f) { m_currentFrame = f; }
    int currentFrame() const { return m_currentFrame; }
    int frameCount() const { return static_cast<int>(m_frames.size()); }
    bool isPlaying() const { return m_playing; }

    void setLooping(bool l) { m_looping = l; }

    virtual void update(float dt);
    virtual void render(Renderer& renderer, int scrollX);

    bool hitTest(int worldX, int worldY) const;
    SDL_Rect worldRect() const;

protected:
    float m_x = 0, m_y = 0;
    int m_width = 0, m_height = 0;
    bool m_visible = true;

    std::vector<SDL_Texture*> m_frames;
    int m_currentFrame = 0;
    float m_frameTimer = 0.0f;
    float m_frameRate = 8.0f;
    bool m_playing = false;
    bool m_looping = true;
};
