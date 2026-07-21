// window_interface.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#include <vector>

namespace xge
{

class WindowInterface
{
public:
    virtual ~WindowInterface() = default;

    virtual bool isOpen() const = 0;
    virtual void display() = 0;
    virtual void clear() = 0;
    virtual void close() = 0;
    virtual InputEvents pollEvents() = 0;
    virtual KeyState getKeyState() const = 0;

    // Render a single game object - delegated to backend
    virtual void renderGameObject(const GameObject& obj) = 0;
};

enum class KeyCode
{
    W,
    A,
    S,
    D,
    Up,
    Down,
    Left,
    Right,
    Escape,
    Space,
    Unknown
};

enum class InputEventType
{
    KeyPressed,
    KeyReleased
};

struct InputEvent
{
    InputEventType type;
    KeyCode key;
};

using InputEvents = std::vector<InputEvent>;

// Key state - tracks which keys are currently held down
struct KeyState
{
    bool keys[10] = { false };  // W, A, S, D, Up, Down, Left, Right, Escape, Space

    int getKeyIndex(KeyCode code) const
    {
        return static_cast<int>(code);
    }

    bool isKeyDown(KeyCode code) const
    {
        int idx = getKeyIndex(code);
        if (idx >= 0 && idx < 10)
            return keys[idx];
        return false;
    }

    void setKeyDown(KeyCode code, bool down)
    {
        int idx = getKeyIndex(code);
        if (idx >= 0 && idx < 10)
            keys[idx] = down;
    }
};

} // namespace xge
