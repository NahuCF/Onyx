#pragma once

#include <string>
#include <memory>
#include <vector>
#include <stack>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace MMO {

class WorldObject;
class EditorWorld;

// Base class for all undoable commands
class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

// Transform change command (for whole objects)
class TransformCommand : public EditorCommand {
public:
    TransformCommand(WorldObject* object,
                     const glm::vec3& oldPos, const glm::quat& oldRot, float oldScale,
                     const glm::vec3& newPos, const glm::quat& newRot, float newScale);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    WorldObject* m_Object;
    glm::vec3 m_OldPosition, m_NewPosition;
    glm::quat m_OldRotation, m_NewRotation;
    float m_OldScale, m_NewScale;
};

class StaticObject;  // Forward declaration

// Mesh transform command (for individual meshes within a model)
class MeshTransformCommand : public EditorCommand {
public:
    MeshTransformCommand(StaticObject* object, const std::string& meshName,
                         const glm::vec3& oldOffset, const glm::vec3& oldRotation, float oldScale,
                         const glm::vec3& newOffset, const glm::vec3& newRotation, float newScale);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    StaticObject* m_Object;
    std::string m_MeshName;
    glm::vec3 m_OldOffset, m_NewOffset;
    glm::vec3 m_OldRotation, m_NewRotation;  // Euler angles in degrees
    float m_OldScale, m_NewScale;
};

// Create object command
class CreateObjectCommand : public EditorCommand {
public:
    CreateObjectCommand(EditorWorld* world, WorldObject* object);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    EditorWorld* m_World;
    WorldObject* m_Object;
    uint64_t m_ObjectGuid;
};

// Delete object command
class DeleteObjectCommand : public EditorCommand {
public:
    DeleteObjectCommand(EditorWorld* world, WorldObject* object);

    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;

private:
    EditorWorld* m_World;
    uint64_t m_ObjectGuid;
    // Store object data for recreation
    std::string m_ObjectName;
    glm::vec3 m_Position;
    glm::quat m_Rotation;
    float m_Scale;
    int m_ObjectType;
};

// Command history manager
class CommandHistory {
public:
    static CommandHistory& Get() {
        static CommandHistory instance;
        return instance;
    }

    void Execute(std::unique_ptr<EditorCommand> command);
    void AddWithoutExecute(std::unique_ptr<EditorCommand> command);  // For already-applied changes
    void Undo();
    void Redo();

    bool CanUndo() const { return !m_UndoStack.empty(); }
    bool CanRedo() const { return !m_RedoStack.empty(); }

    void Clear();

    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;

private:
    CommandHistory() = default;

    std::vector<std::unique_ptr<EditorCommand>> m_UndoStack;
    std::vector<std::unique_ptr<EditorCommand>> m_RedoStack;

    static constexpr size_t MaxHistorySize = 100;
};

} // namespace MMO
