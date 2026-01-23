#pragma once

#include <string>

namespace MMO {

class EditorWorld;
class PanelManager;

class EditorPanel {
    friend class PanelManager;

public:
    virtual ~EditorPanel() = default;

    virtual void OnInit() {}
    virtual void OnImGuiRender() = 0;

    void SetWorld(EditorWorld* world) { m_World = world; }
    EditorWorld* GetWorld() const { return m_World; }

    bool IsOpen() const { return m_IsOpen; }
    void SetOpen(bool open) { m_IsOpen = open; }
    const std::string& GetName() const { return m_Name; }

protected:
    EditorWorld* m_World = nullptr;
    std::string m_Name;
    bool m_IsOpen = true;
};

} // namespace MMO
