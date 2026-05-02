# Coding Conventions

## Naming

| Kind | Style | Examples |
|---|---|---|
| Member variables | `m_PascalCase` | `m_Position`, `m_Health` |
| Methods | `PascalCase` | `GetPosition()`, `ProcessInput()` |
| Local variables / parameters | `camelCase` | `playerPosition`, `deltaTime` |
| Constants / enum values | `UPPER_SNAKE_CASE` | `MAX_PLAYERS`, `TICK_RATE` |
| Types (classes, structs, enums) | `PascalCase` | `MapInstance`, `AuraType` |

## Namespaces

- **Engine**: `Onyx`
- **Editor3D**: `MMO` (top-level) and `Editor3D` (terrain subsystem)
- **MMOGame Shared/Server/Client**: top-level (no enclosing namespace) for game-side code

## Class structure

```cpp
class Example {
public:
    // Public methods first
    void PublicMethod();
    int GetValue() const { return m_Value; }

private:
    // Private methods
    void HelperMethod();

    // Private member variables
    int m_Value;
    std::string m_Name;
};
```

## Style

- `const` wherever possible.
- References over pointers when null is not a valid state.
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`) for ownership; raw pointers for non-owning views.
- Keep methods small and focused on a single responsibility.
- `Texture::Bind(slotNumber)` plus `Shader::SetInt("u_Sampler", slotNumber)` — no `SetBool`; pass `0`/`1` to `SetInt` for booleans.
- `IndexBuffer` constructor takes **byte count**, not element count: `new IndexBuffer(data, count * sizeof(uint32_t))`.

## File organization

- Header-only types (PODs, simple structs) live in `*.h`.
- Implementation lives in `*.cpp` next to the header.
- One main type per `.h`/`.cpp` pair when practical.
- Editor3D adds new files manually to its `CMakeLists.txt` (explicit list); the engine's `Onyx/CMakeLists.txt` uses `GLOB_RECURSE` so new files are picked up on re-configure.
