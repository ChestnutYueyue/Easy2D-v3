#include <easy2d/easy2d.h>
#include <easy2d/utils/logger.h>
#include <cmath>
#include <sstream>

using namespace easy2d;

// ============================================================================
// 旋转方块节点 - 简单的演示节点
// ============================================================================
class RotatingBox : public Node {
public:
    RotatingBox(float size, const Color& color)
        : size_(size), color_(color) {
    }

    Rect getBoundingBox() const override {
        Vec2 pos = getPosition();
        return Rect(pos.x - size_/2, pos.y - size_/2, size_, size_);
    }

    void onUpdate(float dt) override {
        rotationAngle_ += 90.0f * dt;
        if (rotationAngle_ >= 360.0f) rotationAngle_ -= 360.0f;
        setRotation(rotationAngle_);
    }

    void onDraw(RenderBackend& renderer) override {
        Vec2 pos = getPosition();
        Rect rect(pos.x - size_/2, pos.y - size_/2, size_, size_);

        // 绘制填充矩形
        renderer.fillRect(rect, color_);

        // 绘制边框
        renderer.drawRect(rect, Colors::White, 2.0f);
    }

private:
    float size_;
    Color color_;
    float rotationAngle_ = 0.0f;
};

// ============================================================================
// 多线程测试场景
// ============================================================================
class MultiThreadTestScene : public Scene {
public:
    MultiThreadTestScene() = default;
    ~MultiThreadTestScene() override = default;

    void onEnter() override {
        E2D_LOG_INFO("MultiThreadTestScene::onEnter");

        setBackgroundColor(Color(0.05f, 0.05f, 0.1f, 1.0f));

        auto& app = Application::instance();
        float centerX = app.getConfig().width / 2.0f;
        float centerY = app.getConfig().height / 2.0f;

        // 创建旋转的中心方块
        centerBox_ = makePtr<RotatingBox>(100.0f, Color(0.2f, 0.6f, 1.0f, 0.8f));
        centerBox_->setPosition(Vec2(centerX, centerY));
        addChild(centerBox_);

        // 创建周围的静态方块
        std::vector<std::pair<Vec2, Color>> positions = {
            {Vec2(centerX - 200, centerY - 150), Color(0.3f, 1.0f, 0.3f, 0.7f)},
            {Vec2(centerX + 200, centerY - 150), Color(1.0f, 0.3f, 0.3f, 0.7f)},
            {Vec2(centerX - 200, centerY + 150), Color(0.3f, 0.3f, 1.0f, 0.7f)},
            {Vec2(centerX + 200, centerY + 150), Color(1.0f, 1.0f, 0.3f, 0.7f)},
        };

        for (const auto& [pos, color] : positions) {
            auto box = makePtr<RotatingBox>(60.0f, color);
            box->setPosition(pos);
            addChild(box);
        }

        E2D_LOG_INFO("Scene initialized with 5 boxes");
    }

    void onRender(RenderBackend& renderer) override {
        Scene::onRender(renderer);
        drawInfo(renderer);
    }

private:
    void drawInfo(RenderBackend& renderer) {
        // 简单的文字信息（使用简单矩形模拟）
        auto& app = Application::instance();

        // 绘制 FPS 信息
        std::stringstream ss;
        ss << "FPS: " << app.fps();

        // 简单的矩形和线条来表示信息
        renderer.drawRect(Rect(10, 10, 200, 50), Colors::White, 1.0f);
        renderer.drawLine(Vec2(20, 30), Vec2(180, 30), Colors::Green, 1.0f);
    }

    Ptr<RotatingBox> centerBox_;
};

// ============================================================================
// 主函数
// ============================================================================
int main() {
    Logger::init();
    Logger::setLevel(LogLevel::Debug);

    E2D_LOG_INFO("========================");
    E2D_LOG_INFO("Multi-Threaded Rendering Test");
    E2D_LOG_INFO("========================");

    auto& app = Application::instance();

    AppConfig config;
    config.title = "Multi-Threaded Rendering Test";
    config.width = 800;
    config.height = 600;
    config.vsync = true;
    config.fpsLimit = 60;
    config.multiThreadedRendering = true;  // 启用多线程渲染

    if (!app.init(config)) {
        E2D_LOG_ERROR("Failed to initialize application!");
        return -1;
    }

    app.enterScene(makePtr<MultiThreadTestScene>());

    E2D_LOG_INFO("Starting main loop...");

    app.run();

    E2D_LOG_INFO("Application finished.");

    Logger::shutdown();

    return 0;
}
