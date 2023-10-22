#pragma once

#include "Player.h"
#include "Actions/Action.h"

class Entity;

typedef std::shared_ptr<Entity> EntityPtr;

class Entity
{
public:
    std::weak_ptr<Player>& GetPlayer() { return m_Player; }
    void SetPlayer(const std::shared_ptr<Player>& player) { m_Player = player; }

    lptm::Vector2D GetPosition() const { return m_Position; }
    void SetPosition(lptm::Vector2D position)  { m_Position = position; }

    float GetSpeed() const { return m_Speed; }

    void AddAction(ActionPtr action) { m_ActionQueue.push_back(action); }
    void SetColor(lptm::Vector4D color) { m_Color = color; }

    uint32_t GetVision() const { return m_TileVision; }

    virtual void Update() = 0;
    virtual void Render(std::shared_ptr<se::Renderer2D> renderer) = 0;
protected:
    std::weak_ptr<Player> m_Player;
    std::vector<ActionPtr> m_ActionQueue;

    lptm::Vector2D m_Position = { 0.0f, 0.0f };
    lptm::Vector2D m_Velocity = { 0.005f, 0.005f };
    lptm::Vector4D m_Color = { 1.0f, 0.0f, 0.0f, 1.0f};
    float m_Speed = 0.005f;

    uint32_t m_TileVision = 5;  // Range vision of the entity expressed in tile
};