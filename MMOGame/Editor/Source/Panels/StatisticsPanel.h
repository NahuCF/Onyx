#pragma once

#include "EditorPanel.h"

namespace MMO {

class StatisticsPanel : public EditorPanel {
public:
    StatisticsPanel();
    void OnImGuiRender(bool& isOpen) override;
};

} // namespace MMO
