# Writing Plugins

Plugins are shared libraries (`.so` / `.dll`) loaded by the engine and editor
at startup. They let you extend Velix without modifying the engine source:
add custom tools, panels, components, asset importers, and more.

There are two plugin types:

| Type | Base class | Loaded by |
|---|---|---|
| Engine plugin | `elix::sdk::VXPlugin` | Engine + Editor |
| Editor plugin | `elix::sdk::IEditorPlugin` | Editor only |

---

## Project Structure

A plugin is a standalone CMake project that links against `VelixSDK`:

```
MyPlugin/
  CMakeLists.txt
  MyPlugin.cpp
```

**CMakeLists.txt:**

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(VelixSDK CONFIG REQUIRED)

add_library(MyPlugin SHARED MyPlugin.cpp)
target_link_libraries(MyPlugin PRIVATE VelixSDK::VelixSDK)
```

Build the plugin and drop the resulting `.so` / `.dll` into the engine's
`plugins/` directory. The engine loads every library it finds there on startup.

---

## Engine Plugin

An engine plugin runs in both the editor and exported game builds.
Use it to register components, hook into loading, or extend the scripting system.

```cpp
// MyPlugin.cpp
#include "VelixSDK/Plugin.hpp"
#include "Engine/PluginSystem/ComponentRegistry.hpp"

class MyPlugin : public elix::sdk::VXPlugin
{
public:
    const char* getName()    const override { return "MyPlugin"; }
    const char* getVersion() const override { return "1.0.0"; }

    void onLoad() override
    {
        // Register a custom component so it appears in the editor
        elix::engine::ComponentRegistry::instance().registerComponent(
            "Health",
            "Gameplay",
            [](elix::engine::Entity* e,
               elix::engine::Scene*,
               elix::engine::ComponentAddContext& ctx)
            {
                e->addComponent<HealthComponent>();
                ctx.showSuccess("Health added.");
            }
        );
    }

    void onUnload() override
    {
        elix::engine::ComponentRegistry::instance().clear();
    }
};

REGISTER_PLUGIN(MyPlugin)
```

`REGISTER_PLUGIN(ClassName)` generates the required `createPlugin` and
`destroyPlugin` C symbols that the engine uses to instantiate your plugin.

---

## Editor Plugin

An editor plugin runs only in the editor. It gets an `onEditorFrame` callback
every ImGui frame, so you can draw custom panels, overlays, gizmos, and tool
windows using the Dear ImGui API.

```cpp
// MyEditorPlugin.cpp
#include "VelixSDK/EditorPlugin.hpp"
#include <imgui.h>

class MyEditorPlugin : public elix::sdk::IEditorPlugin
{
public:
    const char* getName()    const override { return "MyEditorPlugin"; }
    const char* getVersion() const override { return "1.0.0"; }

    // Optional: add a button to the editor toolbar
    const char* getToolbarButtonLabel() const override { return "My Tool"; }
    void toggleToolbarWindow() override { m_open = !m_open; }

    void onLoad()   override {}
    void onUnload() override {}

    void onEditorFrame(elix::sdk::EditorContext& ctx) override
    {
        if (!m_open) return;

        ImGui::Begin("My Tool", &m_open);

        if (ctx.selectedEntity)
        {
            ImGui::Text("Selected: %s", ctx.selectedEntity->getName().c_str());
            ImGui::Text("Delta time: %.4f s", ctx.deltaTime);
        }
        else
        {
            ImGui::TextDisabled("No entity selected.");
        }

        ImGui::End();
    }

private:
    bool m_open{true};
};

REGISTER_EDITOR_PLUGIN(MyEditorPlugin)
```

`REGISTER_EDITOR_PLUGIN` generates the C symbols just like `REGISTER_PLUGIN`,
but the editor also casts the plugin to `IEditorPlugin` to call `onEditorFrame`.

---

## EditorContext

`EditorContext` is passed to `onEditorFrame` every frame.

| Field | Type | Description |
|---|---|---|
| `scene` | `Scene*` | Active scene; may be null |
| `selectedEntity` | `Entity*` | Currently selected entity; may be null |
| `editorCamera` | `const Camera*` | The editor viewport camera |
| `projectRootPath` | `const filesystem::path*` | Absolute path to the open project |
| `deltaTime` | `float` | Seconds since last frame |
| `wantsBrushInput` | `bool` | Set to `true` to receive brush stroke events |
| `brushStrokeActive` | `bool` | True while the user is painting |
| `brushNdcPosition` | `glm::vec2` | Brush position in normalised device coordinates |

---

## Combining Engine and Editor Behaviour

Inherit from `IEditorPlugin` to get both engine plugin callbacks (`onLoad`,
`onUnload`) and editor callbacks (`onEditorFrame`). The engine plugin system
calls `onLoad` / `onUnload`; the editor additionally calls `onEditorFrame`.

```cpp
class FullPlugin : public elix::sdk::IEditorPlugin
{
public:
    const char* getName()    const override { return "FullPlugin"; }
    const char* getVersion() const override { return "1.0.0"; }

    void onLoad() override
    {
        // Register components, scripts, etc.
    }

    void onUnload() override {}

    void onEditorFrame(elix::sdk::EditorContext& ctx) override
    {
        // Draw ImGui windows
    }
};

REGISTER_EDITOR_PLUGIN(FullPlugin)
```

---

## Tips

- **Keep `onLoad` fast.** It runs on the main thread before the first frame.
- **Do not store raw pointers** to `EditorContext` fields between frames — they
  can change when the scene is reloaded.
- **`clear()` the ComponentRegistry** in `onUnload` only if your plugin owns
  all registered entries. If multiple plugins register components, coordinate
  so you only remove your own.
- Plugins in `plugins/` are loaded in filesystem order. If your plugin depends
  on another, prefix the filenames with a number (e.g. `01_Core.so`, `02_MyPlugin.so`).
