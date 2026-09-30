# AutoClicker

一个用于 Windows 的自动点击器。v3.0 起使用 **Tauri 2（Rust + WebView2）** 重写，
彻底解决了旧版在高分屏上字体发虚、控件错位的问题。

## 为什么重写

旧版是纯 Win32 + GDI 实现。GDI 是位图光栅渲染器，没有亚像素布局，也不具备
现代 DPI 感知能力；在 150% / 200% 缩放下，窗口会被系统位图拉伸，表现为：

- 字体发虚模糊
- 控件大小与布局错乱
- 跨不同 DPI 显示器移动窗口时不重新布局

新版改用 WebView2 渲染界面：

- **矢量渲染 + Per-Monitor v2 DPI 感知** — 文字在任何缩放比例下都是原生清晰度
- 窗口跨屏移动时实时重排，布局始终正确
- 界面用 CSS 实现圆角、阴影、平滑过渡与深色 / 浅色主题

## 功能

- **点击频率**：固定间隔，或围绕基准值上下随机浮动（防检测）
- **点击位置**：坐标位置 / 指针位置 / 顺序循环 / 随机循环，支持范围随机抖动
- **多点点击**：每个坐标可单独设置间隔
- **重复次数**：无限制 / 按次数 / 按时长
- **防沉迷休息**：工作一段时间后自动暂停休息
- **点击方式**：左键 / 右键 / 中键 / 滚轮上 / 滚轮下，单击或双击
- **配置方案**：保存、加载、删除多套配置
- **全局热键**：一键启停（默认 F6，可自定义）
- **系统托盘**：最小化到托盘，托盘菜单快速控制

## 开发

需要 Rust 工具链与 WebView2 运行时（Windows 10/11 自带）。

```bash
# 开发模式（热重载前端）
cargo tauri dev

# 构建发布包
cargo tauri build
```

前端是纯静态的 HTML / CSS / JS，位于 `src/`，**无需 npm 构建步骤**。
它通过 Tauri 全局 API（`window.__TAURI__`）与 Rust 后端通信。

### 目录结构

```
src/                 前端（index.html / styles.css / app.js）
src-tauri/
  src/
    main.rs          入口
    lib.rs           Tauri shell：命令、托盘、坐标拾取钩子
    engine.rs        点击引擎（间隔 / 位置 / 限制 / 防沉迷）
    input.rs         SendInput 鼠标模拟
    hotkey.rs        全局热键（RegisterHotKey）
    settings.rs      设置数据结构
    storage.rs       配置持久化（%APPDATA%\AutoClicker\config.json）
  icons/             应用图标
  tauri.conf.json    应用配置
legacy-cpp/          v2.x 的旧 C++ / Win32 实现，保留供参考
```

配置与配置方案保存在 `%APPDATA%\AutoClicker\config.json`。

## 发布

推送 `v*` 标签即触发 GitHub Actions，自动构建并发布 Windows 安装包。

## 许可

MIT
