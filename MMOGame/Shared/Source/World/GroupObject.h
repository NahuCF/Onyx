#pragma once

#include "WorldObject.h"
#include <vector>
#include <algorithm>

namespace MMO {

class GroupObject : public WorldObject {
public:
    GroupObject(uint64_t guid, const std::string& name = "Group")
        : WorldObject(guid, WorldObjectType::GROUP, name) {}

    // Child management
    void AddChild(uint64_t childGuid) {
        if (std::find(m_Children.begin(), m_Children.end(), childGuid) == m_Children.end()) {
            m_Children.push_back(childGuid);
        }
    }

    void RemoveChild(uint64_t childGuid) {
        m_Children.erase(
            std::remove(m_Children.begin(), m_Children.end(), childGuid),
            m_Children.end()
        );
    }

    // Move child to a new position in the list
    void MoveChild(uint64_t childGuid, size_t newIndex) {
        auto it = std::find(m_Children.begin(), m_Children.end(), childGuid);
        if (it == m_Children.end()) return;

        m_Children.erase(it);
        if (newIndex >= m_Children.size()) {
            m_Children.push_back(childGuid);
        } else {
            m_Children.insert(m_Children.begin() + newIndex, childGuid);
        }
    }

    // Insert child at specific position
    void InsertChildAt(uint64_t childGuid, size_t index) {
        if (std::find(m_Children.begin(), m_Children.end(), childGuid) != m_Children.end()) {
            return;  // Already a child
        }
        if (index >= m_Children.size()) {
            m_Children.push_back(childGuid);
        } else {
            m_Children.insert(m_Children.begin() + index, childGuid);
        }
    }

    bool HasChild(uint64_t childGuid) const {
        return std::find(m_Children.begin(), m_Children.end(), childGuid) != m_Children.end();
    }

    const std::vector<uint64_t>& GetChildren() const { return m_Children; }
    size_t GetChildCount() const { return m_Children.size(); }
    bool IsEmpty() const { return m_Children.empty(); }

    const char* GetTypeName() const override { return "Group"; }

    std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override {
        auto copy = std::make_unique<GroupObject>(newGuid, GetName() + " Copy");

        // Copy transform
        copy->SetPosition(GetPosition());
        copy->SetRotation(GetRotation());
        copy->SetScale(GetScale());

        // Note: Children are NOT cloned - they reference other objects by GUID
        // When duplicating a group, caller must handle cloning children and updating refs

        return copy;
    }

private:
    std::vector<uint64_t> m_Children;
};

} // namespace MMO
