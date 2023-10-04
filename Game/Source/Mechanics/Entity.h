#pragma once

#include "Player.h"
#include "Actions/Action.h"

class Entity
{
public:
    std::weak_ptr<Player>& GetPlayer() { return m_Player; }
    void SetPlayer(const std::shared_ptr<Player>& player) { m_Player = player; }

    lptm::Vector2D GetPosition() const { return m_Position; }
    lptm::Vector2D SetPosition(lptm::Vector2D position)  { m_Position = position; }

    void AddAction(ActionPtr action) { m_ActionQueue.push_back(action); }

    virtual void Update() = 0;
protected:
    std::weak_ptr<Player> m_Player;
    std::vector<ActionPtr> m_ActionQueue;

    lptm::Vector2D m_Position;
    lptm::Vector2D m_Velocity = { 1.0f, 1.0f };
};