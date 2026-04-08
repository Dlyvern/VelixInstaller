# SDK Overview

The Velix SDK is a single header that pulls in everything you need for game logic:

```cpp
#include "Engine/SDK/VelixSDK.hpp"
```

Add it to your `CMakeLists.txt` target and link against `VelixSDK::VelixSDK`.
The SDK exposes five main areas: actors, characters, game state, input, and time.

---

## VXActor

`VXActor` is the standard base class for all game logic. Subclass it, override
the lifecycle methods, attach it to an entity with `REGISTER_SCRIPT`, and the
engine manages the rest.

```cpp
#include "Engine/SDK/VelixSDK.hpp"

class Turret : public elix::engine::VXActor
{
public:
    void onStart() override;
    void onUpdate(float dt) override;
};
REGISTER_SCRIPT(Turret)
```

### Lifecycle

| Method | When called |
|---|---|
| `onStart()` | Once when the entity enters the scene |
| `onUpdate(float dt)` | Every frame; `dt` is seconds since last frame |
| `onStop()` | Once when the entity is removed from the scene |

### Transform

```cpp
glm::vec3 pos = getPosition();
setPosition({0, 1, 0});
translate({0, 0, -1});                        // relative move
rotate(90.f, {0, 1, 0});                      // degrees around axis

glm::vec3 fwd   = getForward();
glm::vec3 right = getRight();
glm::vec3 up    = getUp();
```

World-space variants (`getWorldPosition`, `setWorldPosition`, etc.) are available
for entities that are children of other entities.

### Components

```cpp
auto* rb = getComponent<RigidBodyComponent>();
if (rb) rb->applyImpulse({0, 5, 0});

if (!hasComponent<AudioComponent>())
    addComponent<AudioComponent>();
```

### Scene Queries

```cpp
// Find any entity by name
elix::engine::Entity* door = findEntityByName("MainDoor");

// Find the first actor of a given type in the scene
auto* player = findActor<PlayerController>();

// Find an actor on a specific named entity
auto* boss = findActor<BossAI>("FinalBoss");
```

### Spawning and Destroying

```cpp
// Spawn a new entity with a script attached; onStart() is called immediately
auto* bullet = VXActor::spawn<Bullet>("Bullet_01");
bullet->setPosition(getWorldPosition() + getForward() * 1.5f);

// Destroy this actor's entity
destroy();
```

### Logging

```cpp
log("Player entered the room");
logWarning("Health below 20 %");
logError("Failed to load save file");
```

---

## VXCharacter

`VXCharacter` extends `VXActor` with automatic physics-based movement via
`CharacterMovementComponent`. Use it for any entity that walks, runs, or jumps.

```cpp
class PlayerController : public elix::engine::VXCharacter
{
public:
    void onUpdate(float dt) override
    {
        auto& input = elix::engine::InputManager::instance();

        glm::vec3 dir{0};
        if (input.isKeyDown(elix::engine::KeyCode::W)) dir += getForward();
        if (input.isKeyDown(elix::engine::KeyCode::S)) dir -= getForward();
        if (input.isKeyDown(elix::engine::KeyCode::A)) dir -= getRight();
        if (input.isKeyDown(elix::engine::KeyCode::D)) dir += getRight();

        if (glm::length(dir) > 0.01f)
            move(glm::normalize(dir) * 5.0f, dt);

        if (input.isKeyJustPressed(elix::engine::KeyCode::Space) && isGrounded())
            getMovement().jump(6.0f);
    }
};
REGISTER_SCRIPT(PlayerController)
```

### Key Methods

| Method | Description |
|---|---|
| `move(dir, dt)` | Physics move; `dir` is world-space, pre-scaled by speed |
| `teleport(pos)` | Instant position set, bypasses physics |
| `isGrounded()` | True when touching a surface |
| `getMovement()` | Direct access to `CharacterMovementComponent` |

---

## VXGameState

Global singleton for scene management, time, and render settings.

```cpp
auto& gs = elix::engine::VXGameState::get();

// Scene loading
gs.loadScene("Resources/Scenes/Level2.scene");
gs.loadSceneAdditive("Resources/Scenes/HUD.scene");

// Persistence across scene loads
gs.setDontDestroyOnLoad(getEntity());

// Time
float dt    = gs.getDeltaTime();
float total = gs.getTimeSinceStart();
gs.setTimeScale(0.5f);  // slow-motion

// Render quality at runtime
gs.getRenderSettings().enableBloom = false;
gs.setFXAA(true);
```

---

## InputManager

Poll-based input for keyboard and mouse. Call from `onUpdate`.

```cpp
auto& input = elix::engine::InputManager::instance();

// Keyboard
input.isKeyDown(elix::engine::KeyCode::W);          // held
input.isKeyJustPressed(elix::engine::KeyCode::E);   // pressed this frame
input.isKeyJustReleased(elix::engine::KeyCode::E);  // released this frame

// Mouse buttons  (0=Left, 1=Right, 2=Middle)
input.isMouseButtonJustPressed(0);

// Mouse position and movement
glm::vec2 pos   = input.getMousePosition();   // pixels, top-left origin
glm::vec2 delta = input.getMouseDelta();      // movement since last frame
float     scroll = input.getScrollDelta();    // positive = scroll up
```

---

## Time

Static helpers, usable anywhere without a reference.

```cpp
float dt      = elix::engine::Time::deltaTime();
float total   = elix::engine::Time::totalTime();
uint64_t frame = elix::engine::Time::frameCount();
float scale   = elix::engine::Time::timeScale();
elix::engine::Time::setTimeScale(1.0f);
```
