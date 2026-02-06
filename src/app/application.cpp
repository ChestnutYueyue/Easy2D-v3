#include <easy2d/app/application.h>
#include <easy2d/platform/window.h>
#include <easy2d/platform/input.h>
#include <easy2d/audio/audio_engine.h>
#include <easy2d/scene/scene_manager.h>
#include <easy2d/resource/resource_manager.h>
#include <easy2d/utils/timer.h>
#include <easy2d/event/event_queue.h>
#include <easy2d/event/event_dispatcher.h>
#include <easy2d/graphics/camera.h>
#include <easy2d/graphics/render_backend.h>
#include <easy2d/threading/render_thread.h>
#include <easy2d/threading/frame_data.h>
#include <easy2d/utils/logger.h>
#include <GLFW/glfw3.h>

namespace easy2d {

Application& Application::instance() {
    static Application instance;
    return instance;
}

Application::~Application() {
    shutdown();
}

bool Application::init(const AppConfig& config) {
    if (initialized_) {
        E2D_LOG_WARN("Application already initialized");
        return true;
    }

    config_ = config;

    // 初始化 GLFW
    if (!glfwInit()) {
        E2D_LOG_ERROR("Failed to initialize GLFW");
        return false;
    }

    // 创建窗口
    window_ = makeUnique<Window>();
    WindowConfig winConfig;
    winConfig.title = config.title;
    winConfig.width = config.width;
    winConfig.height = config.height;
    winConfig.fullscreen = config.fullscreen;
    winConfig.vsync = config.vsync;
    winConfig.msaaSamples = config.msaaSamples;
    
    if (!window_->create(winConfig)) {
        E2D_LOG_ERROR("Failed to create window");
        glfwTerminate();
        return false;
    }

    // 初始化渲染器
    renderer_ = RenderBackend::create(config.renderBackend);
    if (!renderer_ || !renderer_->init(window_.get())) {
        E2D_LOG_ERROR("Failed to initialize renderer");
        window_->destroy();
        glfwTerminate();
        return false;
    }

    // 初始化其他子系统
    sceneManager_ = makeUnique<SceneManager>();
    resourceManager_ = makeUnique<ResourceManager>();
    timerManager_ = makeUnique<TimerManager>();
    eventQueue_ = makeUnique<EventQueue>();
    eventDispatcher_ = makeUnique<EventDispatcher>();
    camera_ = makeUnique<Camera>(0, static_cast<float>(config.width),
                                  static_cast<float>(config.height), 0);

    // 初始化多线程渲染（如果启用）
    if (config.multiThreadedRendering) {
        frameDataBuffer_ = makeUnique<FrameDataBuffer>();
        renderThread_ = makeUnique<RenderThread>();

        if (!renderThread_->start(window_->getNativeHandle(), window_.get())) {
            E2D_LOG_WARN("Render thread failed to start, using single-threaded rendering");
            renderThread_.reset();
            frameDataBuffer_.reset();
        } else {
            multiThreadedRendering_ = true;
        }
    }

    // 初始化音频引擎
    AudioEngine::getInstance().initialize();

    initialized_ = true;
    running_ = true;

    E2D_LOG_INFO("Application initialized successfully");
    return true;
}

void Application::shutdown() {
    if (!initialized_) return;

    E2D_LOG_INFO("Shutting down application...");

    // 停止渲染线程（先于其他清理）
    if (renderThread_) {
        renderThread_->stop();
        renderThread_.reset();
    }
    frameDataBuffer_.reset();

    // 清理子系统
    sceneManager_.reset();
    resourceManager_.reset();
    timerManager_.reset();
    eventQueue_.reset();
    eventDispatcher_.reset();
    camera_.reset();

    // 关闭音频
    AudioEngine::getInstance().shutdown();

    // 关闭渲染器
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }

    // 销毁窗口
    if (window_) {
        window_->destroy();
        window_.reset();
    }

    // 终止 GLFW
    glfwTerminate();

    initialized_ = false;
    running_ = false;

    E2D_LOG_INFO("Application shutdown complete");
}

void Application::run() {
    if (!initialized_) {
        E2D_LOG_ERROR("Application not initialized");
        return;
    }

    lastFrameTime_ = glfwGetTime();
    
    while (running_ && !window_->shouldClose()) {
        mainLoop();
    }
}

void Application::quit() {
    shouldQuit_ = true;
    running_ = false;
}

void Application::pause() {
    if (!paused_) {
        paused_ = true;
        E2D_LOG_INFO("Application paused");
    }
}

void Application::resume() {
    if (paused_) {
        paused_ = false;
        lastFrameTime_ = glfwGetTime();  // 重置时间避免大 delta
        E2D_LOG_INFO("Application resumed");
    }
}

void Application::mainLoop() {
    // 计算 delta time
    double currentTime = glfwGetTime();
    deltaTime_ = static_cast<float>(currentTime - lastFrameTime_);
    lastFrameTime_ = currentTime;
    
    // FPS 限制
    if (config_.fpsLimit > 0) {
        float targetFrameTime = 1.0f / config_.fpsLimit;
        while (deltaTime_ < targetFrameTime) {
            currentTime = glfwGetTime();
            deltaTime_ = static_cast<float>(currentTime - lastFrameTime_);
        }
    }

    totalTime_ += deltaTime_;

    // 计算 FPS
    frameCount_++;
    fpsTimer_ += deltaTime_;
    if (fpsTimer_ >= 1.0f) {
        currentFps_ = frameCount_;
        frameCount_ = 0;
        fpsTimer_ -= 1.0f;
    }

    // 处理窗口事件
    window_->pollEvents();

    // 处理事件队列
    if (eventDispatcher_ && eventQueue_) {
        eventDispatcher_->processQueue(*eventQueue_);
    }

    // 更新
    if (!paused_) {
        update();
    }

    // 渲染
    if (multiThreadedRendering_ && renderThread_) {
        // 多线程模式：准备帧数据并提交到渲染线程
        prepareFrameData();
        renderThread_->submitFrame(*frameDataBuffer_->getWriteBuffer());
        frameDataBuffer_->swap();
    } else {
        // 单线程模式：直接渲染
        render();
    }
}

void Application::update() {
    // 更新定时器
    if (timerManager_) {
        timerManager_->update(deltaTime_);
    }

    // 更新场景
    if (sceneManager_) {
        sceneManager_->update(deltaTime_);
    }
}

void Application::render() {
    if (!renderer_) return;

    // 开始帧
    renderer_->beginFrame(Colors::Black);
    
    // 设置视口和相机
    renderer_->setViewport(0, 0, window_->getWidth(), window_->getHeight());
    renderer_->setViewProjection(camera_->getViewProjectionMatrix());

    // 渲染场景
    if (sceneManager_) {
        sceneManager_->render(*renderer_);
    }

    // 结束帧
    renderer_->endFrame();
    
    // 交换缓冲区
    window_->swapBuffers();
}

Input& Application::input() {
    return *window_->getInput();
}

AudioEngine& Application::audio() {
    return AudioEngine::getInstance();
}

SceneManager& Application::scenes() {
    return *sceneManager_;
}

ResourceManager& Application::resources() {
    return *resourceManager_;
}

TimerManager& Application::timers() {
    return *timerManager_;
}

EventQueue& Application::eventQueue() {
    return *eventQueue_;
}

EventDispatcher& Application::eventDispatcher() {
    return *eventDispatcher_;
}

Camera& Application::camera() {
    return *camera_;
}

void Application::enterScene(Ptr<Scene> scene) {
    enterScene(scene, nullptr);
}

void Application::enterScene(Ptr<Scene> scene, Ptr<class Transition> transition) {
    if (sceneManager_ && scene) {
        // 设置场景的视口大小
        scene->setViewportSize(static_cast<float>(config_.width),
                               static_cast<float>(config_.height));
        sceneManager_->enterScene(scene, transition);
    }
}

void Application::prepareFrameData() {
    if (!frameDataBuffer_) return;

    FrameData* frameData = frameDataBuffer_->getWriteBuffer();
    frameData->clear();

    // 捕获相机和视口信息
    frameData->viewProjectionMatrix = camera_->getViewProjectionMatrix();
    frameData->viewportX = 0;
    frameData->viewportY = 0;
    frameData->viewportWidth = window_->getWidth();
    frameData->viewportHeight = window_->getHeight();

    // 设置清空颜色
    if (sceneManager_) {
        Ptr<Scene> activeScene = sceneManager_->getCurrentScene();
        if (activeScene) {
            frameData->clearColor = activeScene->getBackgroundColor();
        }
    }

    frameData->frameNumber = frameCount_;
}

} // namespace easy2d
