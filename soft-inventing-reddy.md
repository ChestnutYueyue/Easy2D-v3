# Easy2D v3.0 全面重构计划

## 重构目标
1. **跨平台渲染** — 用 OpenGL 替换 Direct2D，通过渲染抽象接口支持未来扩展其他后端
2. **现代化架构** — 场景图 + 智能指针 + 空间索引（四叉树/空间哈希）
3. **全面重写** — 在新项目 `Easy2D-v3` 中从头重写所有模块，不污染旧项目
4. **代码质量** — 统一命名规范、添加测试框架、跨平台 String 类

## 关键约定
- **不需要向后兼容**，API 完全重新设计
- **新项目位置**：`C:\Users\soulcoco\Desktop\Easy2D-v4`（独立于旧项目）
- **Application 单例**：Meyer's 单例模式（`static Application& instance()`）
- **渲染后端**：定义 `RenderBackend` 抽象接口，OpenGL 3.3 为第一个实现
- **空间索引**：四叉树 + 空间哈希，支持碰撞检测和场景裁剪，可手动/自动切换策略
- **事件分发**：使用事件队列模式
- **文字渲染**：支持批渲染和 SDF（Signed Distance Field）
- **String 类**：自定义跨平台 String，支持 UTF-8/UTF-16/UTF-32/GBK 等编码转换，无平台特定代码

## 可复用的第三方库（从旧项目复制）
- GLFW（窗口管理）、GLEW（OpenGL 加载）、GLM（数学库）
- stb_image / stb_truetype（图像/字体加载）
- miniaudio（音频）、spdlog（日志）、SimpleIni（数据持久化）

---

## 新项目目录结构

```
Easy2D-v3/
  include/easy2d/
    easy2d.h                          # 统一入口头文件
    core/
      types.h                         # 基础类型、智能指针别名
      string.h                        # 跨平台 String 类（多编码转换）
      color.h                         # 颜色
      math_types.h                    # Vec2, Size, Rect, Transform2D
    platform/
      window.h                        # Window 类（GLFW 封装）
      input.h                         # Input 类
    graphics/
      render_backend.h                # 渲染后端抽象接口
      opengl/
        gl_renderer.h                 # OpenGL 渲染器实现
        gl_shader.h                   # Shader 管理
        gl_texture.h                  # OpenGL 纹理
        gl_sprite_batch.h             # 批量精灵渲染
        gl_shape_renderer.h           # 图形渲染
        gl_text_renderer.h            # 文字渲染（批渲染 + SDF）
      camera.h                        # 2D 相机/视口
      texture.h                       # 纹理抽象接口
      font.h                          # 字体/FontAtlas 接口
    scene/
      node.h                          # 节点基类
      scene.h                         # 场景
      sprite.h                        # 精灵节点
      text.h                          # 文字节点
      shape_node.h                    # 形状节点
      scene_manager.h                 # 场景管理
      transition.h                    # 场景过渡
    spatial/
      spatial_index.h                 # 空间索引抽象接口
      quadtree.h                      # 四叉树
      spatial_hash.h                  # 空间哈希
      spatial_manager.h               # 空间管理器（自动/手动切换策略）
    action/
      action.h                        # 动作基类
      actions.h                       # 所有具体动作
      ease.h                          # 缓动函数
    audio/
      audio_engine.h                  # 音频引擎
      sound.h                         # 音效/音乐
    event/
      event.h                         # 事件类型
      event_queue.h                   # 事件队列
      event_dispatcher.h              # 事件分发器
      input_codes.h                   # 键盘/鼠标码
    resource/
      resource_manager.h              # 资源管理器
    utils/
      logger.h                        # 日志
      random.h                        # 随机数
      timer.h                         # 定时器
      data.h                          # 数据持久化
    app/
      application.h                   # Application 单例
  src/
    core/ platform/ graphics/opengl/ scene/ spatial/
    action/ audio/ event/ resource/ utils/ app/
  third_party/
    glew/ glfw/ glm/ stb/ miniaudio/ spdlog/ simpleini/
  tests/
    core/ graphics/ scene/ spatial/ action/ event/ utils/
  examples/
    hello_world/
    pushbox/
    flappybird/
  xmake.lua
```

---

## 分阶段实施计划

### 第一阶段：项目初始化 + 基础层

**目标**：创建新项目，建立跨平台基础类型

#### 1.1 项目初始化
- 在 `C:\Users\soulcoco\Desktop\` 下创建 `Easy2D-v3` 目录
- 初始化 git 仓库
- 创建目录结构
- 从旧项目复制第三方库到 `third_party/`
- 编写 `xmake.lua`：
  - 静态库目标 `easy2d`，C++17
  - 链接 OpenGL（Windows: opengl32, Linux: GL, macOS: OpenGL.framework）
  - GLEW_STATIC 宏
  - Catch2 测试目标
  - 跨平台 GLFW 源码编译配置

#### 1.2 核心类型 `core/types.h`
```cpp
namespace easy2d {
    template<typename T> using Ptr = std::shared_ptr<T>;
    template<typename T> using UniquePtr = std::unique_ptr<T>;
    template<typename T> using WeakPtr = std::weak_ptr<T>;
    template<typename T, typename... Args>
    Ptr<T> makePtr(Args&&... args);
}
```

#### 1.3 跨平台 String 类 `core/string.h`
- 内部统一 UTF-8 存储
- 支持编码转换（无平台特定 API，纯 C++ 实现）：
  - UTF-8 ↔ UTF-16 ↔ UTF-32（用 `<codecvt>` 或手写转换）
  - GBK ↔ UTF-8（内置 GBK 码表或使用轻量级转换库）
- 接口示例：
```cpp
class String {
public:
    String();
    String(const char* utf8);
    String(const std::string& utf8);
    String(const wchar_t* wide);
    String(const char16_t* utf16);
    String(const char32_t* utf32);

    // 编码转换
    std::string toUtf8() const;
    std::u16string toUtf16() const;
    std::u32string toUtf32() const;
    std::wstring toWide() const;
    std::string toGBK() const;

    static String fromGBK(const char* gbk);
    static String fromUtf16(const char16_t* utf16);

    // 基础操作
    size_t length() const;      // Unicode 字符数
    size_t byteSize() const;    // UTF-8 字节数
    bool empty() const;
    const char* c_str() const;
    const std::string& str() const;

    // 操作符
    String operator+(const String& other) const;
    bool operator==(const String& other) const;
    // ...
private:
    std::string data_; // UTF-8 内部存储
};
```

#### 1.4 颜色 `core/color.h`
- float r/g/b/a，constexpr 命名颜色常量
- 支持 uint32_t RGB/RGBA 构造
- toVec4() 返回 glm::vec4

#### 1.5 数学类型 `core/math_types.h`
- Vec2、Size、Rect — GLM 薄封装
- Transform2D — 内部 glm::mat4，支持 translation/rotation/scaling/skewing
- 无任何平台特定类型

#### 1.6 日志 `utils/logger.h`
- spdlog 薄封装
- E2D_LOG / E2D_WARN / E2D_ERROR 宏

**本阶段产出文件**：
```
xmake.lua
include/easy2d/core/types.h
include/easy2d/core/string.h     + src/core/string.cpp
include/easy2d/core/color.h      + src/core/color.cpp
include/easy2d/core/math_types.h + src/core/math_types.cpp
include/easy2d/utils/logger.h    + src/utils/logger.cpp
include/easy2d/event/input_codes.h
```

---

### 第二阶段：渲染后端抽象 + OpenGL 实现

**目标**：定义渲染抽象接口，实现 OpenGL 3.3 后端，支持精灵批渲染和 SDF 文字

#### 2.1 渲染后端抽象接口 `graphics/render_backend.h`
```cpp
class RenderBackend {
public:
    virtual ~RenderBackend() = default;
    virtual bool init(Window& window) = 0;
    virtual void shutdown() = 0;
    virtual void beginFrame(const Color& clearColor) = 0;
    virtual void endFrame() = 0;
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void setVSync(bool enabled) = 0;

    // 纹理
    virtual UniquePtr<TextureHandle> createTexture(int w, int h,
        const uint8_t* pixels, int channels) = 0;

    // 精灵批渲染
    virtual void beginSpriteBatch(const glm::mat4& viewProjection) = 0;
    virtual void drawSprite(const TextureHandle& tex, const Rect& dest,
        const Rect& src, const Color& tint, const Transform2D& transform) = 0;
    virtual void endSpriteBatch() = 0;

    // 形状渲染
    virtual void drawLine(const Vec2& a, const Vec2& b,
        const Color& color, float width) = 0;
    virtual void fillRect(const Rect& rect, const Color& color) = 0;
    virtual void drawCircle(const Vec2& center, float radius,
        const Color& color, float width, int segments) = 0;
    virtual void fillCircle(const Vec2& center, float radius,
        const Color& color, int segments) = 0;
    // ...

    // 文字渲染
    virtual void drawText(const FontAtlas& font, const String& text,
        const Vec2& position, const Color& color) = 0;

    // 工厂方法
    static UniquePtr<RenderBackend> create(BackendType type);
};

enum class BackendType { OpenGL /*, Vulkan, Metal, D3D11 */ };
```

#### 2.2 OpenGL 实现 `graphics/opengl/`

**gl_shader.h / gl_shader.cpp**：
- GLSL 编译、链接、uniform 设置
- 内置着色器源码（字符串常量）：
  - 精灵着色器（纹理 + 颜色混合 + 透明度）
  - 形状着色器（纯色 + 线段）
  - SDF 文字着色器（距离场渲染，支持描边和阴影）

**gl_texture.h / gl_texture.cpp**：
- stb_image 加载 → glGenTextures/glTexImage2D
- Linear/Nearest 过滤
- 实现 TextureHandle 接口

**gl_sprite_batch.h / gl_sprite_batch.cpp**：
- 顶点格式：position(vec2) + texCoord(vec2) + color(vec4)
- VAO/VBO/EBO，动态顶点缓冲
- 相同纹理合并 draw call，纹理变化或缓冲满时 flush
- 最大批次量 10000 四边形
- 统计信息（draw call 数、四边形数）

**gl_shape_renderer.h / gl_shape_renderer.cpp**：
- 线段、矩形、圆形、多边形、圆角矩形
- 填充和描边

**gl_text_renderer.h / gl_text_renderer.cpp**：
- **FontAtlas**：stb_truetype 生成字形纹理图集
  - 支持 ASCII + CJK Unicode 范围
  - 动态图集扩展（首次使用字符时按需加载）
- **批渲染**：字形四边形复用 SpriteBatch
- **SDF 模式**：
  - 生成 SDF 纹理图集（stb_truetype 支持 SDF 生成）
  - SDF 着色器实现平滑缩放、描边、阴影效果
  - 可通过 FontAtlas::loadSDF() 选择 SDF 模式
- **普通模式**：传统位图字体渲染，适合固定大小文字

**camera.h / camera.cpp**：
- 2D 正交相机：位置、缩放、旋转
- getViewProjectionMatrix() 用于传入渲染器
- screenToWorld() / worldToScreen() 坐标转换

**本阶段产出文件**：
```
include/easy2d/graphics/render_backend.h
include/easy2d/graphics/texture.h          # 纹理抽象接口
include/easy2d/graphics/font.h             # FontAtlas 接口
include/easy2d/graphics/camera.h           + src/graphics/camera.cpp
include/easy2d/graphics/opengl/gl_renderer.h    + src/graphics/opengl/gl_renderer.cpp
include/easy2d/graphics/opengl/gl_shader.h      + src/graphics/opengl/gl_shader.cpp
include/easy2d/graphics/opengl/gl_texture.h     + src/graphics/opengl/gl_texture.cpp
include/easy2d/graphics/opengl/gl_sprite_batch.h + src/graphics/opengl/gl_sprite_batch.cpp
include/easy2d/graphics/opengl/gl_shape_renderer.h + src/graphics/opengl/gl_shape_renderer.cpp
include/easy2d/graphics/opengl/gl_text_renderer.h  + src/graphics/opengl/gl_text_renderer.cpp
src/graphics/opengl/shaders.h              # 内置 GLSL 着色器源码
```

---

### 第三阶段：核心引擎

**目标**：重写窗口/输入/事件队列/场景图/空间索引系统

#### 3.1 窗口 `platform/window.h`
- 实例化类，Config 结构体配置
- GLFW 窗口创建 + OpenGL 3.3 Core Profile 上下文
- 支持 Windows/Linux/macOS 事件轮询
- GLFW 回调 → 推入事件队列
- 光标样式管理

#### 3.2 输入 `platform/input.h`
- 实例化类，绑定到 Window
- 当前帧/上一帧状态数组对比
- isDown/isPressed/isReleased
- 鼠标位置、移动增量、滚轮

#### 3.3 事件队列 `event/event_queue.h`
```cpp
class EventQueue {
public:
    void push(Event event);
    bool poll(Event& event);     // 取出一个事件
    void clear();
    bool empty() const;
    size_t size() const;
private:
    std::queue<Event> queue_;
};
```

#### 3.4 事件分发器 `event/event_dispatcher.h`
```cpp
class EventDispatcher {
public:
    using ListenerId = uint64_t;
    ListenerId on(EventType type, Function<void(Event&)> callback);
    void off(ListenerId id);
    void dispatch(Event& event);
    void processQueue(EventQueue& queue); // 从队列取事件逐个分发
    void clear();
};
```

#### 3.5 场景图 `scene/node.h`
- 继承 `enable_shared_from_this<Node>`
- 子节点：`vector<Ptr<Node>>`
- 变换属性：position, scale, rotation, anchor, skew, opacity
- 脏标记 + 缓存世界变换矩阵
- 虚方法：`onUpdate(float dt)`, `onRender(RenderBackend&)`, `onEnterScene()`, `onExitScene()`
- 每节点 EventDispatcher
- 每节点 Action 列表（runAction/stopAllActions/stopAction）
- 节点名称 + 哈希查找
- z-order 排序

#### 3.6 空间索引系统 `spatial/`

**spatial_index.h — 抽象接口**：
```cpp
class SpatialIndex {
public:
    virtual ~SpatialIndex() = default;
    virtual void insert(Node* node) = 0;
    virtual void remove(Node* node) = 0;
    virtual void update(Node* node) = 0;  // 节点移动后更新
    virtual void clear() = 0;

    // 查询
    virtual std::vector<Node*> query(const Rect& area) const = 0;  // 区域查询
    virtual std::vector<std::pair<Node*, Node*>> queryCollisions() const = 0; // 碰撞对
    virtual Node* queryPoint(const Vec2& point) const = 0;  // 点查询

    virtual void rebuild() = 0;  // 完全重建
};
```

**quadtree.h — 四叉树**：
- 适合稀疏分布、大小不均匀的场景
- 自动分裂/合并策略
- 最大深度限制

**spatial_hash.h — 空间哈希**：
- 适合密集分布、大小相近的对象
- 网格大小可配置
- O(1) 插入/删除

**spatial_manager.h — 空间管理器**：
```cpp
class SpatialManager {
public:
    enum class Strategy { QuadTree, SpatialHash, Auto };

    void setStrategy(Strategy strategy);
    Strategy currentStrategy() const;

    // Auto 模式：根据节点数量和分布自动切换
    // - 节点稀疏/大小差异大 → 四叉树
    // - 节点密集/大小接近 → 空间哈希
    void autoOptimize();

    // 场景裁剪：返回视口内可见节点
    std::vector<Node*> getVisibleNodes(const Rect& viewport) const;

    // 碰撞检测：返回碰撞对
    std::vector<std::pair<Node*, Node*>> getCollisions() const;

    // 点查询
    Node* nodeAtPoint(const Vec2& point) const;

    void update(); // 每帧更新
private:
    UniquePtr<SpatialIndex> index_;
    Strategy strategy_ = Strategy::Auto;
};
```

#### 3.7 场景和场景管理
- `scene/scene.h` — 继承 Node，拥有 Camera 和 SpatialManager
- `scene/scene_manager.h` — 实例化，场景栈，enter/back/clear，过渡协调
- `scene/transition.h` — FadeTransition, MoveTransition, BoxTransition

#### 3.8 节点子类
- `scene/sprite.h` — onRender() 调用 backend.drawSprite()
- `scene/text.h` — onRender() 调用 backend.drawText()
- `scene/shape_node.h` — onRender() 调用 backend 形状方法

#### 3.9 资源管理器 `resource/resource_manager.h`
- 纹理/字体/音效集中加载和缓存
- WeakPtr 缓存，无引用自动清理
- 搜索路径管理

**本阶段产出文件**：
```
include/easy2d/platform/window.h       + src/platform/window.cpp
include/easy2d/platform/input.h        + src/platform/input.cpp
include/easy2d/event/event.h
include/easy2d/event/event_queue.h     + src/event/event_queue.cpp
include/easy2d/event/event_dispatcher.h + src/event/event_dispatcher.cpp
include/easy2d/scene/node.h            + src/scene/node.cpp
include/easy2d/scene/scene.h           + src/scene/scene.cpp
include/easy2d/scene/sprite.h          + src/scene/sprite.cpp
include/easy2d/scene/text.h            + src/scene/text.cpp
include/easy2d/scene/shape_node.h      + src/scene/shape_node.cpp
include/easy2d/scene/scene_manager.h   + src/scene/scene_manager.cpp
include/easy2d/scene/transition.h      + src/scene/transition.cpp
include/easy2d/spatial/spatial_index.h
include/easy2d/spatial/quadtree.h      + src/spatial/quadtree.cpp
include/easy2d/spatial/spatial_hash.h  + src/spatial/spatial_hash.cpp
include/easy2d/spatial/spatial_manager.h + src/spatial/spatial_manager.cpp
include/easy2d/resource/resource_manager.h + src/resource/resource_manager.cpp
```

---

### 第四阶段：游戏系统

**目标**：重写动作/音频/定时器/数据持久化

#### 4.1 动作系统 `action/`
- `Action` 基类（shared_ptr 管理），由 Node 直接管理
- `FiniteAction` → `onProgress(float t)` 归一化时间
- 具体动作：MoveBy/MoveTo, ScaleBy/ScaleTo, RotateBy/RotateTo, FadeIn/FadeOut
- 组合动作：Sequence, Spawn, Loop, Delay, CallFunc
- **缓动函数** `ease.h`：
  - Linear, EaseIn, EaseOut, EaseInOut
  - QuadIn/Out, CubicIn/Out, ElasticIn/Out, BounceIn/Out, BackIn/Out
  - `EaseAction` 包装器：`makePtr<EaseAction>(innerAction, EaseType::BounceOut)`

#### 4.2 音频系统 `audio/`
- AudioEngine — miniaudio 封装，Meyer's 单例或由 Application 拥有
- Sound — play/pause/resume/stop，音量控制
- 仅文件加载（删除 Win32 资源加载）

#### 4.3 工具类 `utils/`
- TimerManager — 实例化，add/cancel/pause/resume/update
- DataStore — SimpleIni 封装
- Random — 模板化 range<T>(min, max)

**本阶段产出文件**：
```
include/easy2d/action/action.h    + src/action/action.cpp
include/easy2d/action/actions.h   + src/action/actions.cpp  (所有具体动作)
include/easy2d/action/ease.h      + src/action/ease.cpp
include/easy2d/audio/audio_engine.h + src/audio/audio_engine.cpp
include/easy2d/audio/sound.h      + src/audio/sound.cpp
include/easy2d/utils/timer.h      + src/utils/timer.cpp
include/easy2d/utils/data.h       + src/utils/data.cpp
include/easy2d/utils/random.h     + src/utils/random.cpp
```

---

### 第五阶段：Application 集成层

**目标**：Application Meyer's 单例统一管理所有子系统

#### 5.1 Application 类 `app/application.h`
```cpp
class Application {
public:
    struct Config {
        String title = "Easy2D Game";
        int width = 640, height = 480;
        int fpsLimit = 0;  // 0 = 不限制
        bool vsync = true;
        BackendType renderBackend = BackendType::OpenGL;
    };

    // Meyer's 单例
    static Application& instance();

    bool init(const Config& config);
    void run();
    void quit();
    void pause();
    void resume();

    // 子系统访问
    Window& window();
    RenderBackend& renderer();
    Input& input();
    AudioEngine& audio();
    SceneManager& scenes();
    ResourceManager& resources();
    TimerManager& timers();
    EventQueue& eventQueue();

    // 便捷方法
    void enterScene(Ptr<Scene> scene, Ptr<Transition> transition = nullptr);
    float deltaTime() const;
    float totalTime() const;

private:
    Application() = default;
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void mainLoop();
    // 主循环：
    //   1. window.pollEvents() → 事件入队列
    //   2. eventQueue 处理 → dispatcher 分发
    //   3. input.update()
    //   4. timers.update(dt)
    //   5. sceneManager.update(dt)  (含 node actions 更新)
    //   6. spatialManager.update()  (空间索引更新)
    //   7. renderer.beginFrame() → sceneManager.render() → renderer.endFrame()
    //   8. window.swapBuffers()
};
```

#### 5.2 统一入口 `easy2d.h`
- 包含所有公共头文件

**新 API 用法示例**：
```cpp
#include <easy2d/easy2d.h>
using namespace easy2d;

class GameScene : public Scene {
    void onEnter() override {
        auto& app = Application::instance();
        auto& res = app.resources();

        auto sprite = makePtr<Sprite>(res.loadTexture("player.png"));
        sprite->setPosition({320, 240});
        sprite->setAnchor({0.5f, 0.5f});
        addChild(sprite);

        // SDF 文字
        auto font = res.loadFontSDF("msyh.ttf", 32);
        auto text = makePtr<Text>("你好 Easy2D!", font);
        text->setPosition({10, 10});
        text->setColor(Color::White);
        addChild(text);

        // 带缓动的动画
        sprite->runAction(makePtr<Loop>(
            makePtr<Sequence>(std::vector<Ptr<Action>>{
                makePtr<EaseAction>(makePtr<MoveBy>(1.0f, Vec2{100, 0}), EaseType::EaseInOut),
                makePtr<EaseAction>(makePtr<MoveBy>(1.0f, Vec2{-100, 0}), EaseType::BounceOut)
            })
        ));
    }
};

int main() {
    auto& app = Application::instance();
    if (app.init({.title = "我的游戏", .width = 800, .height = 600})) {
        app.enterScene(makePtr<GameScene>());
        app.run();
    }
    return 0;
}
```

**本阶段产出文件**：
```
include/easy2d/app/application.h + src/app/application.cpp
include/easy2d/easy2d.h
```

---

### 第六阶段：质量与完善

#### 6.1 测试框架
- xmake.lua 添加 Catch2 依赖
- 测试文件：
  - `tests/core/test_string.cpp` — String 编码转换测试
  - `tests/core/test_math_types.cpp` — Vec2/Rect/Transform2D 运算
  - `tests/core/test_color.cpp` — 颜色构造和转换
  - `tests/scene/test_node.cpp` — 节点层级、变换传播
  - `tests/spatial/test_quadtree.cpp` — 四叉树插入/查询/碰撞
  - `tests/spatial/test_spatial_hash.cpp` — 空间哈希
  - `tests/action/test_actions.cpp` — 动作更新/序列/并行/循环/缓动
  - `tests/event/test_event_queue.cpp` — 事件队列入出
  - `tests/event/test_event_dispatcher.cpp` — 事件注册和分发
  - `tests/utils/test_timer.cpp` — 定时器更新

#### 6.2 示例程序
- `examples/hello_world/` — 最简示例
- `examples/pushbox/` — PushBox 迁移
- `examples/flappybird/` — FlappyBird 迁移

---

## 关键架构决策总结

| 决策 | 选择 | 原因 |
|------|------|------|
| 内存管理 | shared_ptr / unique_ptr | 替代手动 GC，安全、标准 |
| 架构模式 | 场景图 | 天然适配 2D 层级，比 ECS 简单 |
| 渲染后端 | RenderBackend 抽象接口 + OpenGL 3.3 实现 | 可扩展，未来可添加 Vulkan/Metal |
| 文字渲染 | stb_truetype，支持批渲染 + SDF | 高质量缩放，描边/阴影效果 |
| 空间索引 | 四叉树 + 空间哈希，可自动切换 | 碰撞检测 + 场景裁剪 |
| 事件系统 | 事件队列 + 分发器 | 解耦生产者和消费者 |
| Application | Meyer's 单例 | 线程安全延迟初始化 |
| 字符串 | 自定义 String 类，内部 UTF-8 | 多编码转换，无平台依赖 |
| 测试框架 | Catch2 | 头文件式，语法简洁 |
| 构建系统 | xmake | 保留现有选择 |
| 新项目 | Easy2D-v3 独立目录 | 不污染旧项目 |

## 验证方式

1. **编译验证**：每个阶段 `xmake build` 确认 Windows/Linux/macOS 编译通过
2. **单元测试**：`xmake run easy2d_tests` 运行 Catch2 测试
3. **渲染验证**：hello_world 示例验证精灵/SDF文字/形状渲染正确
4. **空间索引验证**：测试碰撞检测和视口裁剪的正确性和性能
5. **示例验证**：PushBox 和 FlappyBird 在新 API 下正常运行
6. **跨平台验证**：在 Windows (MSVC/MinGW) 和 Linux 上分别编译运行
