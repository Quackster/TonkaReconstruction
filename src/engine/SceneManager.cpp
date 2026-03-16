#include "SceneManager.h"

void SceneManager::push(std::unique_ptr<Scene> scene) {
    if (!m_stack.empty()) {
        m_stack.back()->onExit();
    }
    m_stack.push_back(std::move(scene));
    m_stack.back()->onEnter();
}

void SceneManager::pop() {
    if (!m_stack.empty()) {
        m_stack.back()->onExit();
        m_stack.pop_back();
        if (!m_stack.empty()) {
            m_stack.back()->onEnter();
        }
    }
}

void SceneManager::replace(std::unique_ptr<Scene> scene) {
    if (!m_stack.empty()) {
        m_stack.back()->onExit();
        m_stack.pop_back();
    }
    m_stack.push_back(std::move(scene));
    m_stack.back()->onEnter();
}

Scene* SceneManager::current() {
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

void SceneManager::pushDeferred(std::unique_ptr<Scene> scene) {
    m_pending.push_back({PendingOp::Push, std::move(scene)});
}

void SceneManager::popDeferred() {
    m_pending.push_back({PendingOp::Pop, nullptr});
}

void SceneManager::replaceDeferred(std::unique_ptr<Scene> scene) {
    m_pending.push_back({PendingOp::Replace, std::move(scene)});
}

void SceneManager::processPending() {
    for (auto& action : m_pending) {
        switch (action.op) {
            case PendingOp::Push:
                push(std::move(action.scene));
                break;
            case PendingOp::Pop:
                pop();
                break;
            case PendingOp::Replace:
                replace(std::move(action.scene));
                break;
        }
    }
    m_pending.clear();
}
