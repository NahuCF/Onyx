#pragma once

namespace MMO {

// ============================================================
// EDITOR PANEL
// Base class for all editor panels
// ============================================================

class EditorPanel {
public:
    virtual ~EditorPanel() = default;

    // Called every frame to render the panel
    // isOpen is a reference - panel can close itself by setting it to false
    virtual void OnImGuiRender(bool& isOpen) = 0;

    // Optional: Handle events
    virtual void OnEvent() {}

    // Optional: Called when entering/exiting the panel's associated mode
    virtual void OnEnter() {}
    virtual void OnExit() {}

    const char* GetID() const { return m_ID; }
    const char* GetName() const { return m_Name; }

protected:
    const char* m_ID = "Panel";
    const char* m_Name = "Panel";
};

} // namespace MMO
