# Custom Components

Components in Velix are data or behaviour containers that live on entities.
The engine ships with built-in components (transform, rigid body, audio, animator,
etc.), and you can register your own through the `ComponentRegistry` — typically
from inside a plugin.

---

## Built-in Components

| Component | Description |
|---|---|
| `Transform3DComponent` | Position, rotation, scale (always present) |
| `RigidBodyComponent` | Physics simulation |
| `CharacterMovementComponent` | Physics-based character movement |
| `MeshComponent` | 3D mesh + material slots |
| `LightComponent` | Point, spot, or directional light |
| `AudioComponent` | FMOD audio source |
| `AnimatorComponent` | Skeletal animation controller |
| `ScriptComponent` | Hosts a `Script` / `VXActor` instance |
| `DecalComponent` | Projects a decal texture onto surfaces |
| `ReflectionProbeComponent` | Local IBL reflection capture |

Access any of them from a script:

```cpp
auto* audio = getComponent<elix::engine::AudioComponent>();
if (audio)
    audio->play();
```

---

## Registering a Custom Component

Custom components are registered via `ComponentRegistry` so they show up in the
editor's **Add Component** panel. Registration is done at startup — the most
natural place is inside a plugin's `onLoad()`.

```cpp
#include "Engine/PluginSystem/ComponentRegistry.hpp"
#include "Engine/Entity.hpp"

// Your component — plain C++ struct/class stored on the entity
struct HealthComponent
{
    float maxHealth{100.0f};
    float currentHealth{100.0f};

    void takeDamage(float amount)
    {
        currentHealth = std::max(0.0f, currentHealth - amount);
    }

    bool isDead() const { return currentHealth <= 0.0f; }
};

// Registration call (call this from your plugin's onLoad):
void registerHealthComponent()
{
    elix::engine::ComponentRegistry::instance().registerComponent(
        "Health",       // display name shown in the editor
        "Gameplay",     // category grouping in the Add Component panel
        [](elix::engine::Entity* entity,
           elix::engine::Scene*  /*scene*/,
           elix::engine::ComponentAddContext& ctx)
        {
            if (entity->hasComponent<HealthComponent>())
            {
                ctx.showWarning("Health component already exists on this entity.");
                ctx.closePopup = false;  // keep the panel open
                return;
            }
            entity->addComponent<HealthComponent>();
            ctx.showSuccess("Health component added.");
        }
    );
}
```

### ComponentAddContext

| Field / Method | Purpose |
|---|---|
| `showSuccess(msg)` | Show a green toast in the editor |
| `showWarning(msg)` | Show a yellow toast |
| `showError(msg)` | Show a red toast |
| `closePopup` | Set to `false` to keep the Add Component panel open |

---

## Accessing Custom Components from Scripts

Components added via `entity->addComponent<T>()` can be retrieved with
`entity->getComponent<T>()` from any script.

```cpp
class DamageOnCollision : public elix::engine::VXActor
{
public:
    VX_VARIABLE_DEFAULT(float, damage, 25.0f);

    void onCollisionEnter(elix::engine::Entity* other,
                          const elix::engine::CollisionInfo&) override
    {
        auto* health = other->getComponent<HealthComponent>();
        if (health)
            health->takeDamage(damage);
    }
};
REGISTER_SCRIPT(DamageOnCollision)
```

---

## Component Categories

Use categories to group your components in the editor panel.
Any string works, but common categories used by built-in components are:

- `"Common"` — everyday use
- `"Physics"` — physics-related
- `"Rendering"` — visual components
- `"Audio"` — sound components
- `"Animation"` — animation components
- `"Plugin"` — default for plugin-added components

Choose a descriptive category name so designers can find your component quickly.
