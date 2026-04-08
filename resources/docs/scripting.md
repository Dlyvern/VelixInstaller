# Writing Scripts

Scripts are C++ classes that attach behaviour to entities. They compile into your
project's `GameModule` shared library and are hot-reloaded by the editor whenever
you rebuild.

---

## Minimal Script

Every script file follows this pattern:

```cpp
// Sources/MyScript.cpp
#include "Engine/SDK/VelixSDK.hpp"

class MyScript : public elix::engine::VXActor
{
public:
    void onStart() override
    {
        log("MyScript started on entity: " + getEntity()->getName());
    }

    void onUpdate(float dt) override
    {
        translate(getForward() * 2.0f * dt);
    }
};

REGISTER_SCRIPT(MyScript)
```

`REGISTER_SCRIPT(ClassName)` generates the registration glue so the editor can
find your class by name and attach it to entities from the UI.

---

## Lifecycle Callbacks

```cpp
void onStart()  override;  // called once when the entity enters the scene
void onUpdate(float dt) override;  // called every frame
void onStop()   override;  // called once when the entity is removed
```

---

## Exposed Variables

Variables declared with `VX_VARIABLE` appear in the editor's Details panel
and are serialized with the scene, so designers can tweak values without
recompiling.

```cpp
class Patrol : public elix::engine::VXActor
{
public:
    VX_VARIABLE(float, speed);                     // default: 0
    VX_VARIABLE_DEFAULT(float, radius, 5.0f);      // default: 5.0
    VX_VARIABLE_DEFAULT(bool,  loop,   true);
    VX_VARIABLE_DEFAULT(std::string, patrolTag, "patrol_point");

    // glm types
    VX_VARIABLE_DEFAULT(glm::vec3, offset, glm::vec3(0, 1, 0));

    void onUpdate(float dt) override
    {
        float r = radius;   // implicit conversion — no .get() needed
        translate(getForward() * (float)speed * dt);
    }
};
REGISTER_SCRIPT(Patrol)
```

Supported types: `bool`, `int32_t`, `float`, `std::string`,
`glm::vec2`, `glm::vec3`, `glm::vec4`, and entity references.

---

## Entity References

Drag an entity from the scene hierarchy onto an entity-typed field in the editor.

```cpp
class Follow : public elix::engine::VXActor
{
public:
    VX_ENTITY_VARIABLE(target);   // shown as an entity slot in the editor

    void onUpdate(float dt) override
    {
        if (!target.get().isValid()) return;

        auto* targetEntity = findEntityByName("Player"); // or resolve via ID
        if (targetEntity)
            setWorldPosition(targetEntity->getWorldPosition());
    }
};
REGISTER_SCRIPT(Follow)
```

---

## Cross-Script Communication

Use `VX_SCRIPT_VARIABLE` to hold a reference to an entity that carries a
specific script, then resolve it with `GET_SCRIPT`:

```cpp
class Gun : public elix::engine::VXActor
{
public:
    VX_SCRIPT_VARIABLE(PlayerController, player);  // entity slot in editor

    void onStart() override
    {
        m_player = GET_SCRIPT(PlayerController, player);
    }

    void onUpdate(float dt) override
    {
        if (!m_player) return;
        glm::vec3 aimDir = m_player->getForward();
        // ...
    }

private:
    PlayerController* m_player{nullptr};
};
REGISTER_SCRIPT(Gun)
```

---

## Physics Callbacks

Add physics callbacks by overriding these methods.
They fire when this entity's rigid body or trigger volume interacts with another.

```cpp
void onCollisionEnter(elix::engine::Entity* other,
                      const elix::engine::CollisionInfo& info) override
{
    log("Hit " + other->getName());
    log("Contact point: " + std::to_string(info.contactPoint.x));
}

void onCollisionStay(elix::engine::Entity* other,
                     const elix::engine::CollisionInfo& info) override {}

void onCollisionExit(elix::engine::Entity* other) override {}

void onTriggerEnter(elix::engine::Entity* other) override
{
    log("Trigger entered by " + other->getName());
}

void onTriggerExit(elix::engine::Entity* other) override {}
```

---

## Complete Example — Rotating Pickup

```cpp
#include "Engine/SDK/VelixSDK.hpp"

class RotatingPickup : public elix::engine::VXActor
{
public:
    VX_VARIABLE_DEFAULT(float, rotateSpeed, 90.0f);  // degrees per second
    VX_VARIABLE_DEFAULT(float, bobHeight,   0.3f);
    VX_VARIABLE_DEFAULT(float, bobSpeed,    2.0f);

    void onStart() override
    {
        m_originY = getPosition().y;
    }

    void onUpdate(float dt) override
    {
        rotate(rotateSpeed * dt, {0, 1, 0});

        float t = elix::engine::Time::totalTime();
        auto pos = getPosition();
        pos.y = m_originY + std::sin(t * bobSpeed) * bobHeight;
        setPosition(pos);
    }

    void onTriggerEnter(elix::engine::Entity* other) override
    {
        log("Picked up by " + other->getName());
        destroy();
    }

private:
    float m_originY{0.0f};
};

REGISTER_SCRIPT(RotatingPickup)
```
