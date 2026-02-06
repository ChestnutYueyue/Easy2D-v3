#include <easy2d/easy2d.h>
#include <iostream>

using namespace easy2d;

int main() {
    // 创建应用配置，启用多线程渲染
    AppConfig config;
    config.title = "Multi-Threaded Rendering Test";
    config.width = 800;
    config.height = 600;
    config.vsync = true;
    config.multiThreadedRendering = true;  // 启用多线程渲染

    // 初始化应用
    auto& app = Application::instance();
    if (!app.init(config)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }

    // 创建测试场景
    auto scene = Scene::create();
    scene->setBackgroundColor(Colors::DarkGray);

    // 添加一个简单的精灵
    auto resourceMgr = &app.resources();
    auto spriteTexture = resourceMgr->loadTexture("assets/icon.png");
    if (spriteTexture) {
        auto sprite = Sprite::create(spriteTexture);
        sprite->setPosition(400, 300);
        scene->addChild(sprite);
        std::cout << "Sprite added to scene" << std::endl;
    } else {
        std::cout << "Warning: Could not load sprite texture (assets/icon.png)" << std::endl;
    }

    // 进入场景
    app.enterScene(scene);

    std::cout << "Starting application with multi-threaded rendering..." << std::endl;
    std::cout << "Close the window to exit." << std::endl;

    // 运行应用
    app.run();

    std::cout << "Application exited." << std::endl;
    return 0;
}
