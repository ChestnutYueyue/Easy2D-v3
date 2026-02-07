# Easy2D-v3 对象生命周期约束

## 1. 所有权边界（必须遵守）

- Scene：由 SceneManager 的场景栈持有（shared_ptr）；进入栈后才算“活跃场景”。切换/退出由 SceneManager 统一负责。
- Node：由 parent 的 children（shared_ptr）拥有；parent 引用为 weak_ptr，避免循环引用。
- Window/Input：Window 独占拥有 Input（unique_ptr）；GLFW 回调上下文通过 GlfwUserPointer 仅保存裸指针，生命周期由 Window 管控。
- 资源（Texture/FontAtlas/Sound）：ResourceManager 返回 shared_ptr，内部用 weak_ptr 缓存；外部持有者决定真实存活时间。

## 2. 禁止做法（容易引入悬空/循环）

- 禁止把 Node 的 parent 改成 shared_ptr（会形成 parent↔child 循环）。
- 禁止在 EventDispatcher 的回调里捕获“会反向持有 Node 的 shared_ptr”（例如把 Ptr<Node> 存到全局容器里但从不释放）。
- 禁止把 GLFWwindow userPointer 指向短生命周期对象；必须经 Window 持有的 GlfwUserPointer 间接访问。

## 3. 事件回调的安全写法（推荐）

- Node 内部回调捕获 this：只要 Node 的存活由 Scene/父节点保证，且不会在 Node 析构后继续 dispatch，即可安全。
- 跨对象回调：优先捕获 WeakPtr，再在回调里 lock() 判空后使用，避免“回调延迟触发”导致访问已销毁对象。

## 4. 与转场/输入路由相关的约束

- SceneManager 的 hover/capture 指针仅做“每帧输入路由缓存”，不参与所有权；转场开始/结束都会清空缓存，避免指向旧场景对象。
- Transition 渲染阶段会短暂修改 Camera 状态（position/zoom/rotation），并在同帧恢复；自定义 Transition 需保证同样的“修改-恢复”对称性。

