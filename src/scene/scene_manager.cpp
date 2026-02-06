#include <easy2d/scene/scene_manager.h>
#include <easy2d/scene/transition.h>
#include <easy2d/graphics/render_backend.h>
#include <easy2d/graphics/render_command.h>
#include <easy2d/utils/logger.h>
#include <algorithm>

namespace easy2d {

SceneManager& SceneManager::getInstance() {
    static SceneManager instance;
    return instance;
}

void SceneManager::runWithScene(Ptr<Scene> scene) {
    if (!scene) {
        return;
    }
    
    if (!sceneStack_.empty()) {
        E2D_LOG_WARN("SceneManager: runWithScene should only be called once");
        return;
    }
    
    scene->onEnter();
    scene->onAttachToScene(scene.get());
    sceneStack_.push(scene);
}

void SceneManager::replaceScene(Ptr<Scene> scene) {
    if (!scene || isTransitioning_) {
        return;
    }
    
    if (sceneStack_.empty()) {
        runWithScene(scene);
        return;
    }
    
    // Pop current scene
    auto oldScene = sceneStack_.top();
    oldScene->onExit();
    oldScene->onDetachFromScene();
    sceneStack_.pop();
    
    // Push new scene
    scene->onEnter();
    scene->onAttachToScene(scene.get());
    sceneStack_.push(scene);
}

void SceneManager::enterScene(Ptr<Scene> scene) {
    if (!scene || isTransitioning_) {
        return;
    }
    
    if (sceneStack_.empty()) {
        runWithScene(scene);
    } else {
        replaceScene(scene);
    }
}

void SceneManager::enterScene(Ptr<Scene> scene, Ptr<class Transition> transition) {
    if (!scene || isTransitioning_) {
        return;
    }
    
    if (transition) {
        transition->start(getCurrentScene(), scene);
        // TODO: 处理过渡效果
        enterScene(scene);
    } else {
        enterScene(scene);
    }
}

void SceneManager::replaceScene(Ptr<Scene> scene, TransitionType transition, float duration) {
    if (!scene || isTransitioning_) {
        return;
    }
    
    if (sceneStack_.empty()) {
        runWithScene(scene);
        return;
    }
    
    startTransition(sceneStack_.top(), scene, transition, duration);
}

void SceneManager::pushScene(Ptr<Scene> scene) {
    if (!scene || isTransitioning_) {
        return;
    }
    
    // Pause current scene
    if (!sceneStack_.empty()) {
        sceneStack_.top()->pause();
    }
    
    // Push new scene
    scene->onEnter();
    scene->onAttachToScene(scene.get());
    sceneStack_.push(scene);
}

void SceneManager::pushScene(Ptr<Scene> scene, TransitionType transition, float duration) {
    if (!scene || isTransitioning_) {
        return;
    }
    
    if (sceneStack_.empty()) {
        runWithScene(scene);
        return;
    }
    
    startTransition(sceneStack_.top(), scene, transition, duration);
}

void SceneManager::popScene() {
    if (sceneStack_.size() <= 1 || isTransitioning_) {
        return;
    }
    
    auto current = sceneStack_.top();
    current->onExit();
    current->onDetachFromScene();
    sceneStack_.pop();
    
    // Resume previous scene
    if (!sceneStack_.empty()) {
        sceneStack_.top()->resume();
    }
}

void SceneManager::popScene(TransitionType transition, float duration) {
    if (sceneStack_.size() <= 1 || isTransitioning_) {
        return;
    }
    
    auto current = sceneStack_.top();
    sceneStack_.pop();
    auto previous = sceneStack_.top();
    sceneStack_.push(current);  // Put it back temporarily
    
    startTransition(current, previous, transition, duration);
}

void SceneManager::popToRootScene() {
    if (sceneStack_.size() <= 1 || isTransitioning_) {
        return;
    }
    
    // Exit all scenes except root
    while (sceneStack_.size() > 1) {
        auto scene = sceneStack_.top();
        scene->onExit();
        scene->onDetachFromScene();
        sceneStack_.pop();
    }
    
    // Resume root
    sceneStack_.top()->resume();
}

void SceneManager::popToRootScene(TransitionType transition, float duration) {
    if (sceneStack_.size() <= 1 || isTransitioning_) {
        return;
    }
    
    // Find root scene
    std::stack<Ptr<Scene>> tempStack;
    while (sceneStack_.size() > 1) {
        tempStack.push(sceneStack_.top());
        sceneStack_.pop();
    }
    
    auto root = sceneStack_.top();
    
    // Put scenes back
    while (!tempStack.empty()) {
        sceneStack_.push(tempStack.top());
        tempStack.pop();
    }
    
    startTransition(sceneStack_.top(), root, transition, duration);
}

void SceneManager::popToScene(const std::string& name) {
    if (isTransitioning_) {
        return;
    }
    
    // Find target scene in stack
    std::stack<Ptr<Scene>> tempStack;
    Ptr<Scene> target = nullptr;
    
    while (!sceneStack_.empty()) {
        auto scene = sceneStack_.top();
        if (scene->getName() == name) {
            target = scene;
            break;
        }
        scene->onExit();
        scene->onDetachFromScene();
        sceneStack_.pop();
    }
    
    if (target) {
        target->resume();
    }
}

void SceneManager::popToScene(const std::string& name, TransitionType transition, float duration) {
    if (isTransitioning_) {
        return;
    }
    
    auto target = getSceneByName(name);
    if (target && target != sceneStack_.top()) {
        startTransition(sceneStack_.top(), target, transition, duration);
    }
}

Ptr<Scene> SceneManager::getCurrentScene() const {
    if (sceneStack_.empty()) {
        return nullptr;
    }
    return sceneStack_.top();
}

Ptr<Scene> SceneManager::getPreviousScene() const {
    if (sceneStack_.size() < 2) {
        return nullptr;
    }
    
    // Copy stack to access second top
    auto tempStack = sceneStack_;
    tempStack.pop();
    return tempStack.top();
}

Ptr<Scene> SceneManager::getRootScene() const {
    if (sceneStack_.empty()) {
        return nullptr;
    }
    
    // Copy stack to access bottom
    auto tempStack = sceneStack_;
    Ptr<Scene> root;
    while (!tempStack.empty()) {
        root = tempStack.top();
        tempStack.pop();
    }
    return root;
}

Ptr<Scene> SceneManager::getSceneByName(const std::string& name) const {
    auto it = namedScenes_.find(name);
    if (it != namedScenes_.end()) {
        return it->second;
    }
    
    // Search in stack
    auto tempStack = sceneStack_;
    while (!tempStack.empty()) {
        auto scene = tempStack.top();
        if (scene->getName() == name) {
            return scene;
        }
        tempStack.pop();
    }
    
    return nullptr;
}

bool SceneManager::hasScene(const std::string& name) const {
    return getSceneByName(name) != nullptr;
}

void SceneManager::update(float dt) {
    if (isTransitioning_) {
        updateTransition(dt);
    }
    
    if (!sceneStack_.empty()) {
        sceneStack_.top()->updateScene(dt);
    }
}

void SceneManager::render(RenderBackend& renderer) {
    if (isTransitioning_ && outgoingScene_) {
        // During transition, render both scenes
        outgoingScene_->renderScene(renderer);
        // For simple fade effect, we could blend incoming over outgoing
        // For now, just render incoming
        if (incomingScene_) {
            incomingScene_->renderScene(renderer);
        }
    } else if (!sceneStack_.empty()) {
        sceneStack_.top()->renderScene(renderer);
    }
}

void SceneManager::collectRenderCommands(std::vector<RenderCommand>& commands) {
    if (isTransitioning_ && outgoingScene_) {
        // During transition, collect commands from both scenes
        outgoingScene_->collectRenderCommands(commands);
        if (incomingScene_) {
            incomingScene_->collectRenderCommands(commands);
        }
    } else if (!sceneStack_.empty()) {
        sceneStack_.top()->collectRenderCommands(commands);
    }
}

void SceneManager::end() {
    while (!sceneStack_.empty()) {
        auto scene = sceneStack_.top();
        scene->onExit();
        scene->onDetachFromScene();
        sceneStack_.pop();
    }
    namedScenes_.clear();
}

void SceneManager::purgeCachedScenes() {
    namedScenes_.clear();
}

void SceneManager::startTransition(Ptr<Scene> from, Ptr<Scene> to, TransitionType type, float duration) {
    isTransitioning_ = true;
    currentTransition_ = type;
    transitionDuration_ = duration;
    transitionElapsed_ = 0.0f;
    outgoingScene_ = from;
    incomingScene_ = to;
}

void SceneManager::updateTransition(float dt) {
    transitionElapsed_ += dt;
    
    float progress = transitionDuration_ > 0.0f 
        ? std::min(1.0f, transitionElapsed_ / transitionDuration_) 
        : 1.0f;
    
    // Apply transition effect based on type
    switch (currentTransition_) {
        case TransitionType::Fade:
            // Fade effect logic would go here
            break;
        case TransitionType::SlideLeft:
        case TransitionType::SlideRight:
        case TransitionType::SlideUp:
        case TransitionType::SlideDown:
            // Slide effect logic would go here
            break;
        case TransitionType::Scale:
            // Scale effect logic would go here
            break;
        case TransitionType::Flip:
            // Flip effect logic would go here
            break;
        default:
            break;
    }
    
    if (progress >= 1.0f) {
        finishTransition();
    }
}

void SceneManager::finishTransition() {
    isTransitioning_ = false;
    
    if (outgoingScene_) {
        outgoingScene_->onExit();
        outgoingScene_->onDetachFromScene();
        
        // Remove outgoing from stack if needed
        if (!sceneStack_.empty() && sceneStack_.top() == outgoingScene_) {
            sceneStack_.pop();
        }
    }
    
    if (incomingScene_) {
        incomingScene_->onEnter();
        incomingScene_->onAttachToScene(incomingScene_.get());
        sceneStack_.push(incomingScene_);
    }
    
    outgoingScene_.reset();
    incomingScene_.reset();
    
    if (transitionCallback_) {
        transitionCallback_();
        transitionCallback_ = nullptr;
    }
}

} // namespace easy2d
