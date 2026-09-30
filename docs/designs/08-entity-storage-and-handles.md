# 08. Entity storage, handles and type identity

**Status:** current code stores objects by value in a vector and finds them by name; the generational-handle design is decided but not implemented

## The problem

Nearly everything in the engine is loaded at run time, so it seems everything must be a pointer. It does not: heap allocation and per-object pointers are different questions. The real questions are how objects refer to each other when entities can be added and removed, and how polymorphism is handled.

## Options considered for storage

| Option | Notes |
|---|---|
| `vector<Entity>` by value | Simple and cache-friendly. Any growth invalidates every pointer, reference and iterator into it. |
| `vector<unique_ptr<Entity>>` | Entities keep a fixed address, so raw observers stay valid across growth. Does not protect against removal. Needed for real polymorphism through virtual functions. |
| Indices | Survive reallocation; silently point at a different entity after a swap-and-pop removal. |
| **Handle `{index, generation}`** | Chosen. Index says where, generation says whether it is still the same thing. Slot reuse bumps the generation, so a stale handle fails loudly instead of touching the wrong object. |
| `std::deque` | `push_back` never invalidates references to existing elements; a cheaper middle ground when entities are mostly added, rarely removed. |

Details worth keeping when it is built:

- Bump the generation on **free**, not on allocate.
- Keep a freelist of freed indices to reuse slots.
- Use `uint32_t` for the generation; `uint16_t` can wrap in practice.
- Handles are plain data and can be copied and stored in other entities freely (an action's target, a projectile's owner).

## Composition vs inheritance

The author's instinct: identity (ID, name) in a thin core, everything else bolted on. The suggested rule is a litmus test: if any entity you can imagine lacks a property, it is a component, not a base-class field. ID, generation and name pass; "renderable", "has health" and "has a collider" do not (a trigger volume has none of them). This matches an entity-component-system layout, and is also why per-entity data is `map<string, Value>` ([07](07-object-variables-and-references.md)).

Data-oriented design (all components of one kind stored contiguously) is the performance argument for components living outside the entity, but it is not a current need.

## Type identity: not an enum

C++ enums are fixed at compile time. If game authors can define new kinds of objects in XML without recompiling the engine, an entity's type should be data:

- a type name from the XML, interned to an integer ID at load for fast comparison, and
- no `switch` over an engine enum for author-defined types.

Real enums remain right for things the engine fixes (its own states, fixed categories the C++ branches on). The rule of thumb: if a game author could ever want to add a new one without touching the engine, it is not an enum.

C++26 reflection does not change this. It is compile-time only; it can generate boilerplate around existing enums but cannot add enumerators from data read at run time, and no runtime reflection is on the committee's table. Wildcard queries over variables are instead just a map scan ([06](06-names-and-classes.md)).

## Current code

`Game` keeps `std::vector<Object>` and finds objects by linear search on name (`getObject`, `tryGetObject`). There is no handle system, no `Value` variant, and no ECS layer yet.

## Second batch: alternatives

- **Opt-in parts.** An object as a name and a position plus optional physics, appearance, input and actions, so that a HUD element carries no velocity or collision. Storage could hold parts in separate arrays. Details in [27](27-object-composition-and-shape.md).
- **Component with state.** A part such as a growth stage that owns its own state fits the same model ([20](20-ideas-parking-lot.md)).

## Sources

- "C++ design conversation A" (2026-08-07).
- "C++ design conversation B" (2026-09-05).
- Second batch: "Declarative Pong XML redesign" (2026-09-23).
