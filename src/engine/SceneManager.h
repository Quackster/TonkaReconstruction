#pragma once

#include <memory>
#include <vector>
#include <functional>

class Renderer;
union SDL_Event;

class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void handleEvent(const SDL_Event& event) {}
    virtual void update(float dt) = 0;
    virtual void render(Renderer& renderer) = 0;
};

class SceneManager {
public:
    void push(std::unique_ptr<Scene> scene);
    void pop();
    void replace(std::unique_ptr<Scene> scene);

    Scene* current();
    int stackSize() const { return static_cast<int>(m_stack.size()); }

    // Deferred operations (processed between frames)
    void pushDeferred(std::unique_ptr<Scene> scene);
    void popDeferred();
    void replaceDeferred(std::unique_ptr<Scene> scene);
    void processPending();

private:
    std::vector<std::unique_ptr<Scene>> m_stack;

    enum class PendingOp { Push, Pop, Replace };
    struct PendingAction {
        PendingOp op;
        std::unique_ptr<Scene> scene;
    };
    std::vector<PendingAction> m_pending;
};
