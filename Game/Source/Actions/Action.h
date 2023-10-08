#pragma once

#include <SE.h>
#include "Enums.h"

class Action;

typedef std::shared_ptr<Action> ActionPtr;

class Action
{
public:
    virtual EventType Update() = 0;
    ~Action() {
        std::cout << "Action finished" << std::endl;
    }

    ActionType GetType() const { return m_Type; }
protected:
    ActionType m_Type;
};