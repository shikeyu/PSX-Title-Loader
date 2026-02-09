# PSX Title Loader / PSX 标题加载器

[English](#english) | [中文](#chinese)

---

<a name="english"></a>
## English

### Project Description
This is a PlayStation 1 (PSX) homebrew application designed to serve as a custom title screen or boot loader for PSX games. It displays a splash screen image loaded from the CD, waits for user input, performs a fade-out effect, and then chains into the main game executable.

### Features
*   **External Resource Loading**: Loads the title image (`TITLE.TIM`) directly from the CD root directory, allowing for easy customization without recompiling.
*   **Auto-Centering**: Automatically centers the image on the screen regardless of its dimensions (handles widths > 256 pixels by splitting sprites).
*   **Fade Effect**: Implements a smooth brightness fade-out effect upon button press.
*   **Game Launcher**: Automatically loads and executes the main game executable (`GAME.EXE`) after the splash screen.
*   **Double Buffering**: Uses standard double-buffered rendering for flicker-free display.

### Prerequisites
*   **Psy-Q SDK**: The official PlayStation 1 development kit (headers and libraries like `libgpu`, `libcd`, `libetc`, etc.).
*   **Compiler**: `ccpsx` (included in Psy-Q).
*   **Tools**: `cpe2x` for converting CPE files to PSX executables.

### File Structure
*   `main.c`: The main source code containing system initialization, image loading, rendering loop, and game launching logic.
*   `Makefile`: Build script for compiling the project.
*   `TITLE.TIM`: (Runtime requirement) The PlayStation TIM format image to be displayed. Must be placed in the CD root.
*   `GAME.EXE`: (Runtime requirement) The target game executable to launch. Must be placed in the CD root.

### Compilation
To compile the project, ensure your environment is set up for Psy-Q development and run:

```bash
make
```

This will generate `main.exe`.

### Usage
1.  Compile the source code to get `main.exe`.
2.  Prepare your CD image layout:
    *   Place `main.exe` as the boot executable (usually defined in `SYSTEM.CNF`).
    *   Place a valid TIM image named `TITLE.TIM` in the root directory.
    *   Place the target game executable named `GAME.EXE` in the root directory.
3.  Build the ISO and test in an emulator or on real hardware.
4.  **Note**: To prevent game data reading errors, please ensure that any new files are added to the end sectors of the disc layout.

---

<a name="chinese"></a>
## 中文

### 项目描述
这是一个 PlayStation 1 (PSX) 自制程序，旨在作为 PSX 游戏的自定义标题画面或引导加载器。它会从光盘加载一张标题图片，等待用户输入，执行淡出效果，然后启动主游戏的可执行文件。

### 功能特点
*   **外部资源加载**：直接从光盘根目录加载标题图片 (`TITLE.TIM`)，无需重新编译即可更换图片。
*   **自动居中**：无论图片尺寸如何，都会自动将其在屏幕上居中显示（支持宽度超过 256 像素的图片，会自动切分 Sprite）。
*   **淡出效果**：按下按键后实现平滑的亮度淡出效果。
*   **游戏启动器**：在标题画面结束后自动加载并执行主游戏文件 (`GAME.EXE`)。
*   **双重缓冲**：使用标准的双重缓冲渲染，防止画面闪烁。

### 前置要求
*   **Psy-Q SDK**：官方 PlayStation 1 开发工具包（包含 `libgpu`、`libcd`、`libetc` 等头文件和库）。
*   **编译器**：`ccpsx`（包含在 Psy-Q 中）。
*   **工具**：`cpe2x`，用于将 CPE 文件转换为 PSX 可执行文件。

### 文件结构
*   `main.c`: 主源代码，包含系统初始化、图片加载、渲染循环和游戏启动逻辑。
*   `Makefile`: 用于编译项目的构建脚本。
*   `TITLE.TIM`: (运行时需求) 需要显示的 PlayStation TIM 格式图片。必须放置在光盘根目录。
*   `GAME.EXE`: (运行时需求) 要启动的目标游戏可执行文件。必须放置在光盘根目录。

### 编译方法
确保你的开发环境已配置好 Psy-Q SDK，然后在命令行运行：

```bash
make
```

这将生成 `main.exe` 文件。

### 使用说明
1.  编译源代码得到 `main.exe`。
2.  准备光盘镜像文件布局：
    *   将 `main.exe` 设置为启动可执行文件（通常在 `SYSTEM.CNF` 中定义）。
    *   将有效的 TIM 图片命名为 `TITLE.TIM` 并放置在根目录。
    *   将目标游戏可执行文件命名为 `GAME.EXE` 并放置在根目录。
3.  构建 ISO 并在模拟器或真机上测试。
4.  **注意**：为了预防游戏数据读取错误，请将新增的文件添加到尾部扇区。
