# AutoClicker - Windows 自动点击工具

一个功能丰富的 Windows 鼠标自动点击工具，使用 C++ 和 Win32 API 开发。

## 功能特性

- **多种点击模式**：左键/右键/中键单击、双击、滚轮上下滚动
- **多坐标点点击**：支持添加多个坐标点，每个点可设置独立间隔
- **点击频率**：固定间隔 或 固定+随机偏移
- **点击位置**：坐标位置 / 指针跟随 / 范围随机抖动
- **循环模式**：按次数 / 按时长 / 无限循环
- **防沉迷**：可设置定时休息
- **配置管理**：保存/加载/删除多个配置方案（INI 持久化）
- **全局热键**：可自定义热键一键启停（默认 F6）
- **系统托盘**：最小化到托盘，右键菜单操作
- **进度条**：实时显示循环进度

## 架构设计

本项目采用 C++ 面向对象设计，实现了**封装、继承、多态**三大特性：

### 多态 (Polymorphism)
- `ClickStrategy` 抽象基类定义统一的 `execute()` 接口
- 7 种具体策略类（左键、右键、双击、滚轮等）各自实现点击逻辑
- `ClickEngine` 通过基类指针统一调度，无需关心具体类型

### 继承 (Inheritance)
- `LeftClickStrategy`、`RightClickStrategy` 等全部继承自 `ClickStrategy`
- 共享基类的接口定义，各自覆写 `execute()` 方法

### 封装 (Encapsulation)
- `MainWindow` 封装窗口生命周期、消息分发和所有 UI 控件
- `ClickEngine` 封装后台线程和调度逻辑
- `TrayIconManager` 封装系统托盘图标
- `HotkeyManager` 封装热键注册和捕获
- `ProfileManager` 封装配置持久化
- `FontManager` / `ProgressBar` 封装资源管理

### 文件结构

```
AutoClicker/
├── .github/workflows/build.yml   # GitHub Actions CI/CD
├── src/
│   ├── main.cpp                  # 程序入口
│   ├── resource.h                # 资源头文件
│   ├── app.rc                    # 资源文件
│   ├── core/                     # 核心逻辑
│   │   ├── ClickStrategy.h       # 抽象基类（多态）
│   │   ├── ClickStrategies.h     # 具体策略（继承）
│   │   ├── ClickPoint.h          # 坐标点数据模型
│   │   ├── ClickSettings.h       # 设置数据模型
│   │   ├── ClickEngine.h/cpp     # 点击引擎（封装）
│   ├── ui/                       # 界面层
│   │   ├── Theme.h               # 布局/颜色/字体常量
│   │   ├── FontManager.h         # 字体管理（封装）
│   │   ├── ProgressBar.h         # 自绘进度条
│   │   ├── UIBuilder.h/cpp       # UI 构建
│   │   └── MainWindow.h/cpp      # 主窗口（封装）
│   ├── utils/                    # 工具层
│   │   ├── TrayIconManager.h     # 系统托盘
│   │   ├── HotkeyManager.h       # 热键管理
│   │   └── ProfileManager.h      # 配置管理
│   └── resources/
│       ├── app.ico               # 应用图标
│       └── small.ico             # 小图标
├── AutoClicker.sln               # VS 解决方案
└── AutoClicker.vcxproj           # VS 项目文件
```

## 编译

### 使用 Visual Studio
1. 打开 `AutoClicker.sln`
2. 选择 `Release | x64` 配置
3. 生成解决方案

### 使用命令行 (MSBuild)
```bash
msbuild AutoClicker.sln /p:Configuration=Release /p:Platform=x64
```

生成的 exe 位于 `x64/Release/AutoClicker.exe`

## 使用方法

1. 运行 `AutoClicker.exe`
2. 设置点击频率、位置、方式等参数
3. 点击"点选坐标"可在屏幕上选取多个点击位置
4. 按 F6（或自定义热键）开始/暂停点击
5. 可保存配置方案供下次使用

## 技术栈

- C++17
- Win32 API (Windows SDK)
- MSBuild / Visual Studio 2022
- GitHub Actions CI/CD

## License

MIT
