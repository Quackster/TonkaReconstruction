#include "GarageScene.h"
#include "engine/Application.h"
#include <cstdio>
#include <cmath>

GarageScene::GarageScene(Application& app)
    : m_app(app) {}

void GarageScene::onEnter() {
    m_background = m_app.assets().loadTexture("assets/images/module12/background.png");
    m_canvas = m_app.assets().loadTexture("assets/images/module12/overlay_020.png");

    setupVehicles();
    setupPaintColors();

    // Create paint surface (render target for spray paint)
    m_paintSurface = SDL_CreateTexture(
        m_app.renderer().sdlRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        288, 194
    );
    SDL_SetTextureBlendMode(m_paintSurface, SDL_BLENDMODE_BLEND);
    // Clear to transparent
    SDL_SetRenderTarget(m_app.renderer().sdlRenderer(), m_paintSurface);
    SDL_SetRenderDrawColor(m_app.renderer().sdlRenderer(), 0, 0, 0, 0);
    SDL_RenderClear(m_app.renderer().sdlRenderer());
    SDL_SetRenderTarget(m_app.renderer().sdlRenderer(), nullptr);

    // Canvas position (centered in the right portion of the garage)
    m_canvasRect = {320, 140, 288, 194};

    auto* ambient = m_app.assets().loadSound("assets/audio/ambient/AMBNT12.WAV");
    if (ambient) {
        m_app.audio().playAmbient(ambient);
    }

    m_selectedVehicle = -1;
    m_painting = false;
    m_strokes.clear();
}

void GarageScene::onExit() {
    m_app.audio().stopAmbient();
    if (m_paintSurface) {
        SDL_DestroyTexture(m_paintSurface);
        m_paintSurface = nullptr;
    }
}

void GarageScene::setupVehicles() {
    m_vehicles.clear();

    // Three vehicles on display in the garage, positioned on the left side
    struct VehicleInfo {
        int overlayIdx;
        SDL_Rect rect;
        const char* name;
        int moduleId;
    };

    VehicleInfo infos[] = {
        {0, {20, 80, 240, 159}, "Dump Truck", 7},
        {1, {30, 260, 176, 175}, "Front Loader", 8},
        {2, {15, 380, 208, 107}, "Grader", 9},
    };

    for (auto& info : infos) {
        char path[256];
        std::snprintf(path, sizeof(path), "assets/images/module12/overlay_%03d.png", info.overlayIdx);

        Vehicle v;
        v.texture = m_app.assets().loadTexture(path);
        v.rect = info.rect;
        v.name = info.name;
        v.moduleId = info.moduleId;
        m_vehicles.push_back(v);
    }
}

void GarageScene::setupPaintColors() {
    m_paintColors.clear();

    // Paint color palette - Tonka's classic colors
    SDL_Color colors[] = {
        {255, 204, 0, 255},    // Tonka Yellow
        {200, 40, 40, 255},    // Red
        {40, 100, 200, 255},   // Blue
        {40, 160, 40, 255},    // Green
        {255, 140, 0, 255},    // Orange
        {160, 40, 160, 255},   // Purple
        {255, 255, 255, 255},  // White
        {40, 40, 40, 255},     // Black
    };

    int startX = 330;
    int startY = 360;
    int size = 28;
    int gap = 6;

    for (int i = 0; i < 8; ++i) {
        int col = i % 4;
        int row = i / 4;
        PaintColor pc;
        pc.color = colors[i];
        pc.rect = {startX + col * (size + gap), startY + row * (size + gap), size, size};
        m_paintColors.push_back(pc);
    }

    m_selectedColor = 0;
}

void GarageScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = m_app.input().mouseX();
        int my = m_app.input().mouseY();

        // Check color palette clicks
        for (int i = 0; i < static_cast<int>(m_paintColors.size()); ++i) {
            SDL_Point pt = {mx, my};
            if (SDL_PointInRect(&pt, &m_paintColors[i].rect)) {
                m_selectedColor = i;
                auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK2.WAV");
                if (click) m_app.audio().playSound(click);
                return;
            }
        }

        // Check vehicle clicks (select for viewing)
        for (int i = 0; i < static_cast<int>(m_vehicles.size()); ++i) {
            SDL_Point pt = {mx, my};
            if (SDL_PointInRect(&pt, &m_vehicles[i].rect)) {
                m_selectedVehicle = i;
                m_strokes.clear();
                // Clear paint surface
                SDL_SetRenderTarget(m_app.renderer().sdlRenderer(), m_paintSurface);
                SDL_SetRenderDrawColor(m_app.renderer().sdlRenderer(), 0, 0, 0, 0);
                SDL_RenderClear(m_app.renderer().sdlRenderer());
                SDL_SetRenderTarget(m_app.renderer().sdlRenderer(), nullptr);

                auto* click = m_app.assets().loadSound("assets/audio/snds/BTNCLIK1.WAV");
                if (click) m_app.audio().playSound(click);
                return;
            }
        }

        // Check canvas click (start painting)
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &m_canvasRect)) {
            m_painting = true;
            applyPaint(mx, my);
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        m_painting = false;
    }

    if (event.type == SDL_MOUSEMOTION && m_painting) {
        int mx = m_app.input().mouseX();
        int my = m_app.input().mouseY();
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &m_canvasRect)) {
            applyPaint(mx, my);
        }
    }
}

void GarageScene::applyPaint(int x, int y) {
    // Convert to canvas-local coordinates
    int cx = x - m_canvasRect.x;
    int cy = y - m_canvasRect.y;
    auto& col = m_paintColors[m_selectedColor].color;

    // Render spray paint dot onto the paint surface
    SDL_SetRenderTarget(m_app.renderer().sdlRenderer(), m_paintSurface);
    SDL_SetRenderDrawBlendMode(m_app.renderer().sdlRenderer(), SDL_BLENDMODE_BLEND);

    // Spray paint effect: multiple semi-transparent dots in a radius
    int radius = 12;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist <= radius) {
                // Alpha falls off with distance from center
                int alpha = static_cast<int>(180.0f * (1.0f - dist / radius));
                if (alpha > 0) {
                    SDL_SetRenderDrawColor(m_app.renderer().sdlRenderer(),
                        col.r, col.g, col.b, static_cast<Uint8>(alpha));
                    SDL_RenderDrawPoint(m_app.renderer().sdlRenderer(), cx + dx, cy + dy);
                }
            }
        }
    }

    SDL_SetRenderTarget(m_app.renderer().sdlRenderer(), nullptr);

    // Play spray sound occasionally
    static int sprayCounter = 0;
    if (sprayCounter++ % 8 == 0) {
        auto* spray = m_app.assets().loadSound("assets/audio/snds/SPRAY.WAV");
        if (spray) m_app.audio().playSound(spray);
    }
}

void GarageScene::update(float /*dt*/) {
    int mx = m_app.input().mouseX();
    int my = m_app.input().mouseY();

    m_hoveredVehicle = -1;
    for (int i = 0; i < static_cast<int>(m_vehicles.size()); ++i) {
        SDL_Point pt = {mx, my};
        if (SDL_PointInRect(&pt, &m_vehicles[i].rect)) {
            m_hoveredVehicle = i;
            break;
        }
    }
}

void GarageScene::render(Renderer& renderer) {
    // Background
    if (m_background) {
        renderer.drawTextureEx(m_background, 0, 0, 640, 480);
    }

    // Vehicles on the left side
    for (int i = 0; i < static_cast<int>(m_vehicles.size()); ++i) {
        auto& v = m_vehicles[i];
        if (v.texture) {
            renderer.drawTextureEx(v.texture, v.rect.x, v.rect.y, v.rect.w, v.rect.h);
        }
        // Highlight hovered vehicle
        if (i == m_hoveredVehicle) {
            SDL_SetRenderDrawBlendMode(renderer.sdlRenderer(), SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 0, 40);
            SDL_RenderFillRect(renderer.sdlRenderer(), &v.rect);
        }
    }

    // Paint canvas (gold background plate)
    if (m_canvas) {
        renderer.drawTextureEx(m_canvas, m_canvasRect.x, m_canvasRect.y,
                              m_canvasRect.w, m_canvasRect.h);
    }

    // Draw the selected vehicle silhouette on canvas
    if (m_selectedVehicle >= 0 && m_vehicles[m_selectedVehicle].texture) {
        auto& v = m_vehicles[m_selectedVehicle];
        // Scale vehicle to fit canvas
        float scale = std::min(
            static_cast<float>(m_canvasRect.w - 20) / v.rect.w,
            static_cast<float>(m_canvasRect.h - 20) / v.rect.h
        );
        int sw = static_cast<int>(v.rect.w * scale);
        int sh = static_cast<int>(v.rect.h * scale);
        int sx = m_canvasRect.x + (m_canvasRect.w - sw) / 2;
        int sy = m_canvasRect.y + (m_canvasRect.h - sh) / 2;
        renderer.drawTextureEx(v.texture, sx, sy, sw, sh);
    }

    // Draw paint strokes on canvas
    if (m_paintSurface) {
        renderer.drawTextureEx(m_paintSurface,
                              m_canvasRect.x, m_canvasRect.y,
                              m_canvasRect.w, m_canvasRect.h);
    }

    // Color palette
    for (int i = 0; i < static_cast<int>(m_paintColors.size()); ++i) {
        auto& pc = m_paintColors[i];
        SDL_SetRenderDrawColor(renderer.sdlRenderer(), pc.color.r, pc.color.g, pc.color.b, 255);
        SDL_RenderFillRect(renderer.sdlRenderer(), &pc.rect);

        // Selection border
        if (i == m_selectedColor) {
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 255, 255, 255, 255);
            SDL_Rect border = {pc.rect.x - 2, pc.rect.y - 2, pc.rect.w + 4, pc.rect.h + 4};
            SDL_RenderDrawRect(renderer.sdlRenderer(), &border);
        } else {
            SDL_SetRenderDrawColor(renderer.sdlRenderer(), 80, 80, 80, 255);
            SDL_RenderDrawRect(renderer.sdlRenderer(), &pc.rect);
        }
    }
}
