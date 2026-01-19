#pragma once

namespace MMO {

class EditorWorld;

class ToolbarPanel {
public:
    void Init(EditorWorld* world);
    void OnImGuiRender();

private:
    EditorWorld* m_World = nullptr;
};

} // namespace MMO
