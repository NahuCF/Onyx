#include "EditorCommand.h"
#include "World/EditorWorld.h"
#include "World/WorldTypes.h"
#include "World/StaticObject.h"
#include <iostream>

namespace MMO {

// ─────────────────────────────────────────────────────────────────────────────
// TransformCommand
// ─────────────────────────────────────────────────────────────────────────────

TransformCommand::TransformCommand(WorldObject* object,
                                   const glm::vec3& oldPos, const glm::quat& oldRot, float oldScale,
                                   const glm::vec3& newPos, const glm::quat& newRot, float newScale)
    : m_Object(object)
    , m_OldPosition(oldPos), m_OldRotation(oldRot), m_OldScale(oldScale)
    , m_NewPosition(newPos), m_NewRotation(newRot), m_NewScale(newScale)
{
    std::cout << "[Command] Created TransformCommand for " << (object ? object->GetName() : "null")
              << " oldPos=(" << oldPos.x << "," << oldPos.y << "," << oldPos.z << ")"
              << " newPos=(" << newPos.x << "," << newPos.y << "," << newPos.z << ")" << std::endl;
}

void TransformCommand::Execute() {
    if (m_Object) {
        m_Object->SetPosition(m_NewPosition);
        m_Object->SetRotation(m_NewRotation);
        m_Object->SetScale(m_NewScale);
    }
}

void TransformCommand::Undo() {
    if (m_Object) {
        std::cout << "[Undo] Restoring " << m_Object->GetName()
                  << " from (" << m_Object->GetPosition().x << "," << m_Object->GetPosition().y << "," << m_Object->GetPosition().z << ")"
                  << " to (" << m_OldPosition.x << "," << m_OldPosition.y << "," << m_OldPosition.z << ")" << std::endl;
        m_Object->SetPosition(m_OldPosition);
        m_Object->SetRotation(m_OldRotation);
        m_Object->SetScale(m_OldScale);
    } else {
        std::cout << "[Undo] ERROR: m_Object is null!" << std::endl;
    }
}

std::string TransformCommand::GetDescription() const {
    return "Transform " + (m_Object ? m_Object->GetName() : "Object");
}

// ─────────────────────────────────────────────────────────────────────────────
// MeshTransformCommand
// ─────────────────────────────────────────────────────────────────────────────

MeshTransformCommand::MeshTransformCommand(StaticObject* object, const std::string& meshName,
                                           const glm::vec3& oldOffset, const glm::vec3& oldRotation, float oldScale,
                                           const glm::vec3& newOffset, const glm::vec3& newRotation, float newScale)
    : m_Object(object)
    , m_MeshName(meshName)
    , m_OldOffset(oldOffset), m_OldRotation(oldRotation), m_OldScale(oldScale)
    , m_NewOffset(newOffset), m_NewRotation(newRotation), m_NewScale(newScale)
{
    std::cout << "[Command] Created MeshTransformCommand for mesh '" << meshName << "'"
              << " oldOffset=(" << oldOffset.x << "," << oldOffset.y << "," << oldOffset.z << ")"
              << " newOffset=(" << newOffset.x << "," << newOffset.y << "," << newOffset.z << ")" << std::endl;
}

void MeshTransformCommand::Execute() {
    if (m_Object) {
        MeshMaterial& mat = m_Object->GetOrCreateMeshMaterial(m_MeshName);
        mat.positionOffset = m_NewOffset;
        mat.rotationOffset = m_NewRotation;
        mat.scaleMultiplier = m_NewScale;
    }
}

void MeshTransformCommand::Undo() {
    if (m_Object) {
        MeshMaterial& mat = m_Object->GetOrCreateMeshMaterial(m_MeshName);
        std::cout << "[Undo] Restoring mesh '" << m_MeshName << "'"
                  << " from offset (" << mat.positionOffset.x << "," << mat.positionOffset.y << "," << mat.positionOffset.z << ")"
                  << " to (" << m_OldOffset.x << "," << m_OldOffset.y << "," << m_OldOffset.z << ")" << std::endl;
        mat.positionOffset = m_OldOffset;
        mat.rotationOffset = m_OldRotation;
        mat.scaleMultiplier = m_OldScale;
    } else {
        std::cout << "[Undo] ERROR: m_Object is null for mesh transform!" << std::endl;
    }
}

std::string MeshTransformCommand::GetDescription() const {
    return "Transform Mesh " + m_MeshName;
}

// ─────────────────────────────────────────────────────────────────────────────
// CreateObjectCommand
// ─────────────────────────────────────────────────────────────────────────────

CreateObjectCommand::CreateObjectCommand(EditorWorld* world, WorldObject* object)
    : m_World(world)
    , m_Object(object)
    , m_ObjectGuid(object ? object->GetGuid() : 0)
{
}

void CreateObjectCommand::Execute() {
    // Object is already created, nothing to do
}

void CreateObjectCommand::Undo() {
    if (m_World && m_ObjectGuid > 0) {
        m_World->DeleteObject(m_ObjectGuid);
    }
}

std::string CreateObjectCommand::GetDescription() const {
    return "Create " + (m_Object ? m_Object->GetName() : "Object");
}

// ─────────────────────────────────────────────────────────────────────────────
// DeleteObjectCommand
// ─────────────────────────────────────────────────────────────────────────────

DeleteObjectCommand::DeleteObjectCommand(EditorWorld* world, WorldObject* object)
    : m_World(world)
    , m_ObjectGuid(object ? object->GetGuid() : 0)
{
    if (object) {
        m_ObjectName = object->GetName();
        m_Position = object->GetPosition();
        m_Rotation = object->GetRotation();
        m_Scale = object->GetScale();
        m_ObjectType = static_cast<int>(object->GetObjectType());
    }
}

void DeleteObjectCommand::Execute() {
    if (m_World && m_ObjectGuid > 0) {
        m_World->DeleteObject(m_ObjectGuid);
    }
}

void DeleteObjectCommand::Undo() {
    // Recreation would need to restore the full object state
    // For now, just a placeholder - full implementation would need object serialization
    // TODO: Implement proper object recreation
}

std::string DeleteObjectCommand::GetDescription() const {
    return "Delete " + m_ObjectName;
}

// ─────────────────────────────────────────────────────────────────────────────
// CommandHistory
// ─────────────────────────────────────────────────────────────────────────────

void CommandHistory::Execute(std::unique_ptr<EditorCommand> command) {
    command->Execute();
    m_UndoStack.push_back(std::move(command));
    m_RedoStack.clear();  // Clear redo stack on new action

    // Limit history size
    if (m_UndoStack.size() > MaxHistorySize) {
        m_UndoStack.erase(m_UndoStack.begin());
    }
}

void CommandHistory::AddWithoutExecute(std::unique_ptr<EditorCommand> command) {
    // Add command to history without executing (for already-applied changes like gizmo drags)
    m_UndoStack.push_back(std::move(command));
    m_RedoStack.clear();

    if (m_UndoStack.size() > MaxHistorySize) {
        m_UndoStack.erase(m_UndoStack.begin());
    }
}

void CommandHistory::Undo() {
    if (m_UndoStack.empty()) return;

    auto command = std::move(m_UndoStack.back());
    m_UndoStack.pop_back();

    command->Undo();
    m_RedoStack.push_back(std::move(command));
}

void CommandHistory::Redo() {
    if (m_RedoStack.empty()) return;

    auto command = std::move(m_RedoStack.back());
    m_RedoStack.pop_back();

    command->Execute();
    m_UndoStack.push_back(std::move(command));
}

void CommandHistory::Clear() {
    m_UndoStack.clear();
    m_RedoStack.clear();
}

std::string CommandHistory::GetUndoDescription() const {
    if (m_UndoStack.empty()) return "";
    return m_UndoStack.back()->GetDescription();
}

std::string CommandHistory::GetRedoDescription() const {
    if (m_RedoStack.empty()) return "";
    return m_RedoStack.back()->GetDescription();
}

} // namespace MMO
