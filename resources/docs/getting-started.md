# Getting Started with Velix

Velix is a C++ game engine built on Vulkan 1.3. This guide walks you through
installing the engine, creating your first project, and getting a scene running.

---

## System Requirements

- **OS**: Linux (Ubuntu 22.04+) or Windows 10/11 (64-bit)
- **GPU**: Vulkan 1.3 capable GPU (NVIDIA GTX 1060+ / AMD RX 5000+ / Intel Arc)
- **RAM**: 8 GB minimum, 16 GB recommended
- **Disk**: 2 GB for the engine, plus space for your project
- **Compiler**: GCC 12+ or MSVC 2022+ (C++20 required)
- **CMake**: 3.16 or newer

---

## Installing Velix

Use the Velix Installer (this application) to download and install the engine.

1. Open the **Versions** tab and select the version you want to install.
2. Click **Install**. The installer downloads and extracts the engine to your chosen directory.
3. Once installed, the version appears in your installed versions list.

---

## Creating Your First Project

1. Open the **Projects** tab.
2. Click **Create Project**.
3. Choose a parent directory (e.g. `~/Documents/ElixProjects`).
4. Enter a project name.
5. Configure project settings — rendering quality, camera defaults, etc.
6. Click **Create Project**.

The installer creates the following project layout:

```
MyProject/
  CMakeLists.txt          # Root build file — finds VelixSDK and builds GameModule
  project.elixproject     # Project descriptor read by the editor
  project.settings        # Render and camera settings
  default_scene.scene     # Starting scene
  Sources/                # Your C++ game code lives here
    GameModule.cpp        # Entry point — exports getScriptsRegister()
  Resources/              # Assets: textures, meshes, audio, materials
  build/                  # CMake build output
  .vscode/                # VS Code tasks for building the game module
```

---

## Opening a Project

Click **Open** on any project card in the **Projects** tab. The installer launches the
Velix Editor with your project loaded.

You can also click **Open Existing** to browse for a `.elixproject` file on disk.

---

## Building Your Game Module

Your game logic compiles into a shared library (`GameModule.so` / `GameModule.dll`).
The editor hot-reloads it automatically when you rebuild.

**From VS Code** (task pre-configured):
Press `Ctrl+Shift+B` → **Build GameModule**.

**From the terminal:**

```bash
cd MyProject
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

The built library is placed in `build/` where the editor picks it up.

---

## Project Descriptor (project.elixproject)

The `.elixproject` file is a JSON file that tells the editor where everything lives.
It is created automatically and rarely needs hand-editing.

```json
{
  "name": "MyProject",
  "path": "/home/you/ElixProjects/MyProject/",
  "scene": "/home/you/ElixProjects/MyProject/default_scene.scene",
  "resources_path": "/home/you/ElixProjects/MyProject/Resources",
  "sources_path": "/home/you/ElixProjects/MyProject/Sources",
  "build_dir": "/home/you/ElixProjects/MyProject/build"
}
```

---

## Next Steps

- **SDK Overview** — learn the core `VXActor`, `VXCharacter`, input, and time APIs.
- **Writing Scripts** — attach behaviour to entities with C++ scripts.
- **Custom Components** — create reusable data components.
- **Writing Plugins** — extend the editor with custom tools.
