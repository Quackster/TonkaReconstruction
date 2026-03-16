#include "Application.h"
#include "game/SignInScene.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>

Application::~Application() {
    m_scenes.reset();
    m_audio.reset();
    m_assets.reset();
    m_renderer.reset();

    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

bool Application::init(const std::string& title, int width, int height) {
    m_nativeW = width;
    m_nativeH = height;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << "\n";
        return false;
    }

    // Create window at 2x scale by default, resizable
    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width * 2, height * 2,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!m_window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->init(m_window, width, height)) {
        return false;
    }

    m_assets = std::make_unique<AssetManager>(m_renderer->sdlRenderer());
    m_audio = std::make_unique<AudioManager>();
    m_input = std::make_unique<InputManager>();
    m_scenes = std::make_unique<SceneManager>();

    if (!m_audio->init()) {
        std::cerr << "Warning: Audio init failed, continuing without sound\n";
    }

    m_text.init(m_renderer->sdlRenderer());

    // Load saved game state
    m_gameState.load("tonka_save.dat");

    // Push initial scene (sign-in screen, like the original)
    m_scenes->push(std::make_unique<SignInScene>(*this));

    m_running = true;
    return true;
}

void Application::run() {
    Uint64 lastTime = SDL_GetPerformanceCounter();
    const Uint64 freq = SDL_GetPerformanceFrequency();

    while (m_running) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - lastTime) / static_cast<float>(freq);
        lastTime = now;

        // Cap delta time to avoid spiral of death
        if (dt > 0.1f) dt = 0.1f;

        handleEvents();
        update(dt);
        render();

        // Cap at ~60fps
        SDL_Delay(1);
    }
}

void Application::quit() {
    m_running = false;
}

void Application::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (m_scenes->stackSize() > 1) {
                        m_scenes->pop();
                    } else {
                        m_running = false;
                    }
                }
                break;
            default:
                break;
        }
        m_input->handleEvent(event, *m_renderer);
        if (m_scenes->current()) {
            m_scenes->current()->handleEvent(event);
        }
    }
}

void Application::update(float dt) {
    m_input->update();
    if (m_scenes->current()) {
        m_scenes->current()->update(dt);
    }
    m_scenes->processPending();
}

void Application::render() {
    m_renderer->clear();
    if (m_scenes->current()) {
        m_scenes->current()->render(*m_renderer);
    }
    m_renderer->present();
}
