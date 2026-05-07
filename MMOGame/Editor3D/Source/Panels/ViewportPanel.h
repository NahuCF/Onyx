#pragma once

#include "EditorPanel.h"
#include "Gizmo/TransformGizmo.h"
#include "Navigation/NavMeshDebugView.h"
#include "Terrain/TerrainMaterialLibrary.h"
#include "World/EditorWorldSystem.h"
#include "World/StaticObject.h"
#include "World/WorldTypes.h"
#include <Graphics/AnimatedModel.h>
#include <Graphics/Animator.h>
#include <Graphics/Buffers.h>
#include <Graphics/Framebuffer.h>
#include <Graphics/Model.h>
#include <Graphics/PostProcess/PostProcessStack.h>
#include <Graphics/SceneRenderer.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace MMO {

	class ViewportPanel : public EditorPanel
	{
	public:
		ViewportPanel();
		~ViewportPanel() override;

		void OnInit() override;
		void OnImGuiRender() override;

		void SetSceneRenderer(Onyx::SceneRenderer* renderer) { m_SceneRenderer = renderer; }

		const glm::vec3& GetCameraPosition() const { return m_CameraPosition; }
		const glm::vec3& GetCameraFront() const { return m_CameraFront; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }

		float GetViewportWidth() const { return m_ViewportWidth; }
		float GetViewportHeight() const { return m_ViewportHeight; }
		bool IsHovered() const { return m_ViewportHovered; }
		bool IsFocused() const { return m_ViewportFocused; }

		const Onyx::RenderStats& GetRenderStats() const { return m_RenderStats; }

		float GetTotalRenderTime() const { return m_TotalRenderTime; }
		float GetSubmitModelsTime() const { return m_SubmitModelsTime; }
		float GetResolveModelTime() const { return m_ResolveModelTime; }
		float GetShadowPassTime() const { return m_ShadowPassTime; }
		float GetTerrainPassTime() const { return m_TerrainPassTime; }
		float GetBatchRenderTime() const { return m_BatchRenderTime; }
		float GetWorldObjectRenderTime() const { return m_WorldObjectRenderTime; }
		bool& GetProfilePassTiming() { return m_ProfilePassTiming; }

		glm::vec3& GetLightDir() { return m_LightDir; }
		glm::vec3& GetLightColor() { return m_LightColor; }
		float& GetAmbientStrength() { return m_AmbientStrength; }
		bool& GetEnableShadows() { return m_EnableShadows; }
		float& GetShadowBias() { return m_ShadowBias; }
		float& GetShadowDistance() { return m_ShadowDistance; }
		uint32_t GetShadowMapSize() const { return m_ShadowMapSize; }
		void SetShadowMapSize(uint32_t size);

		float& GetSplitLambda() { return m_SplitLambda; }
		bool& GetShowCascades() { return m_ShowCascades; }

		bool& GetShowGrid() { return m_ShowGrid; }
		bool& GetShowWireframe() { return m_ShowWireframe; }
		bool& GetEnableMSAA() { return m_EnableMSAA; }
		bool& GetShowNavMesh() { return m_ShowNavMesh; }

		// Reload the navmesh overlay from disk and rebuild GPU buffers.
		// Pass the same Data root + map id used by ExportForRuntime / Bake.
		// Safe to call when no .nav exists — the overlay just disappears.
		void ReloadNavMeshDebugView(const std::string& dataDir, uint32_t mapId);

		Onyx::PostProcessStack& GetPostProcessStack() { return m_PostProcessStack; }

		void FocusOnObject(const WorldObject* object);
		void FocusOnSelection();

		Onyx::Model* GetModel(const std::string& path);

		bool IsAnimatedModel(const std::string& path);
		Onyx::AnimatedModel* GetAnimatedModel(const std::string& path);
		Onyx::Animator* GetAnimator(uint64_t objectGuid);
		void InvalidateAnimator(uint64_t objectGuid);
		void ReloadAnimations(uint64_t objectGuid, StaticObject* object);

		Editor3D::EditorWorldSystem& GetWorldSystem() { return m_WorldSystem; }
		Editor3D::TerrainMaterialLibrary& GetTerrainMaterialLibrary() { return m_TerrainMaterialLibrary; }
		bool IsTerrainEnabled() const { return m_TerrainEnabled; }
		void SetTerrainEnabled(bool enabled) { m_TerrainEnabled = enabled; }

		enum class TerrainToolMode
		{
			Raise,
			Lower,
			Flatten,
			Smooth,
			Paint,
			Hole,
			Ramp
		};

		struct TerrainToolState
		{
			bool toolActive = false;
			TerrainToolMode mode = TerrainToolMode::Raise;

			float brushRadius = 5.0f;
			float brushStrength = 1.0f;
			bool brushValid = false;
			glm::vec3 brushPos = glm::vec3(0.0f);

			float flattenHeight = 0.0f;
			float flattenHardness = 0.5f;

			int paintLayer = 0;
			std::string selectedMaterialId;

			bool rampPlacing = false;
			glm::vec3 rampStart = glm::vec3(0.0f);
			float rampStartHeight = 0.0f;

			bool sobelNormals = true;
			bool smoothNormals = true;
			bool pixelNormals = true;
			bool diamondGrid = true;
			int meshResolution = 65;
			bool pbr = false;
			int debugSplatmap = 0;
		};

		TerrainToolState m_TerrainTool;

	private:
		void RenderScene();
		void RenderGrid();
		void RenderWorldObjects();
		void RenderTerrain();
		void RenderNavMeshOverlay();
		void HandleTerrainInput();
		bool RaycastTerrain(const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& hitPoint);
		void RenderSelectionOutline();
		void RenderGizmoIcons();
		void RenderGizmo();

		// Socket debug overlay: renders colored dots + labels for every socket on
		// the selected entity's model. Validates the importer → .omdl → load →
		// Animator pose → AttachmentSet::ResolveWorld → WorldUIAnchorSystem chain.
		void RenderSocketDebugOverlay(const glm::vec2& vpPos);

		void HandleCameraInput();
		void HandleObjectPicking();
		void HandleGizmoInteraction();
		void UpdateCameraVectors();

		void SubmitModelsToRenderer();

		struct PickResult
		{
			uint64_t objectGuid = 0;
			int meshIndex = -1;
			WorldObjectType type = WorldObjectType::STATIC_OBJECT;
			bool hit = false;
		};

		void RenderPickingPass();
		PickResult ReadPickingBuffer(float mouseX, float mouseY);
		void RenderObjectForPicking(StaticObject* obj);
		void RenderIconForPicking(WorldObject* obj, WorldObjectType type, float size);

		glm::vec3 ScreenToWorldRay(float screenX, float screenY);

		std::unique_ptr<Onyx::Framebuffer> m_Framebuffer;
		std::unique_ptr<Onyx::Framebuffer> m_PickingFramebuffer;

		std::unique_ptr<Onyx::VertexArray> m_BillboardVAO;
		std::unique_ptr<Onyx::VertexBuffer> m_BillboardVBO;
		std::unique_ptr<Onyx::IndexBuffer> m_BillboardEBO;

		std::unique_ptr<Onyx::Texture> m_LightIconTexture;
		std::unique_ptr<Onyx::Texture> m_SpawnIconTexture;
		std::unique_ptr<Onyx::Texture> m_ParticleIconTexture;
		std::unique_ptr<Onyx::Texture> m_PortalIconTexture;
		std::unique_ptr<Onyx::Texture> m_TriggerIconTexture;

		std::unique_ptr<Onyx::Shader> m_ObjectShader;
		std::unique_ptr<Onyx::Shader> m_ModelShader;
		std::unique_ptr<Onyx::Shader> m_SkinnedShader;
		std::unique_ptr<Onyx::Shader> m_InfiniteGridShader;
		std::unique_ptr<Onyx::Shader> m_OutlineShader;
		std::unique_ptr<Onyx::Shader> m_IconShader;
		std::unique_ptr<Onyx::Shader> m_PickingShader;
		std::unique_ptr<Onyx::Shader> m_PickingBillboardShader;
		std::unique_ptr<Onyx::Shader> m_BillboardShader;
		std::unique_ptr<Onyx::Shader> m_ShadowDepthShader;

		std::unique_ptr<Onyx::VertexArray> m_CubeVAO;
		std::unique_ptr<Onyx::VertexBuffer> m_CubeVBO;
		std::unique_ptr<Onyx::IndexBuffer> m_CubeEBO;

		std::unique_ptr<Onyx::VertexArray> m_GridVAO;
		std::unique_ptr<Onyx::VertexBuffer> m_GridVBO;
		std::unique_ptr<Onyx::IndexBuffer> m_GridEBO;

		// Navmesh debug overlay
		std::unique_ptr<Onyx::Shader> m_NavMeshOverlayShader;
		std::unique_ptr<Onyx::VertexArray> m_NavMeshVAO;
		std::unique_ptr<Onyx::VertexBuffer> m_NavMeshVBO;
		std::unique_ptr<Onyx::IndexBuffer> m_NavMeshEBO;
		uint32_t m_NavMeshIndexCount = 0;
		int m_NavMeshPolyCount = 0;
		Editor3D::NavMeshDebugView m_NavMeshDebugView;
		bool m_ShowNavMesh = false;

		glm::vec3 m_CameraPosition = glm::vec3(0.0f, 10.0f, 20.0f);
		glm::vec3 m_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 m_CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 m_CameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
		float m_CameraYaw = -90.0f;
		float m_CameraPitch = -20.0f;
		float m_CameraSpeed = 20.0f;
		float m_CameraSensitivity = 0.15f;

		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);

		float m_ViewportWidth = 800.0f;
		float m_ViewportHeight = 600.0f;
		bool m_ViewportHovered = false;
		bool m_ViewportFocused = false;
		glm::vec2 m_ViewportPos = glm::vec2(0.0f);

		bool m_RightMouseDown = false;
		bool m_MiddleMouseDown = false;
		bool m_FirstMouse = true;
		bool m_WantsContextMenu = false;

		glm::vec3 m_LightDir = glm::vec3(-0.5f, -1.0f, -0.3f);
		glm::vec3 m_LightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		float m_AmbientStrength = 0.3f;
		bool m_ShowGrid = true;
		bool m_ShowWireframe = false;
		bool m_EnableMSAA = true;
		bool m_ShowSocketDebug = false;
		float m_GridSize = 100.0f;
		float m_GridSpacing = 1.0f;

		bool m_EnableDirectionalLight = true;

		bool m_EnableShadows = true;
		float m_ShadowBias = 0.005f;
		float m_ShadowDistance = 60.0f;
		uint32_t m_ShadowMapSize = 2048;
		float m_SplitLambda = 0.0f;
		bool m_ShowCascades = false;

		Onyx::RenderStats m_RenderStats;

		bool m_ProfilePassTiming = false;
		float m_TotalRenderTime = 0.0f;
		float m_SubmitModelsTime = 0.0f;
		float m_ResolveModelTime = 0.0f;
		float m_ShadowPassTime = 0.0f;
		float m_TerrainPassTime = 0.0f;
		float m_BatchRenderTime = 0.0f;
		float m_WorldObjectRenderTime = 0.0f;

		std::unique_ptr<TransformGizmo> m_Gizmo;
		glm::vec3 m_GizmoStartObjectPos = glm::vec3(0.0f);
		glm::quat m_GizmoStartObjectRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		float m_GizmoStartObjectScale = 1.0f;

		glm::vec3 m_GizmoStartMeshOffset = glm::vec3(0.0f);
		glm::vec3 m_GizmoStartMeshRotation = glm::vec3(0.0f);
		float m_GizmoStartMeshScale = 1.0f;
		std::string m_GizmoSelectedMeshName;

		std::unordered_map<uint64_t, std::unique_ptr<Onyx::Animator>> m_AnimatorCache;

		// Persistent pointer caches — avoid repeated AssetManager string-hash lookups
		struct ResolvedModel
		{
			Onyx::Model* staticModel = nullptr;
			Onyx::AnimatedModel* animModel = nullptr;
			bool isAnimated = false;
		};
		std::unordered_map<std::string, ResolvedModel> m_ResolvedModelCache;

		Onyx::SceneRenderer* m_SceneRenderer = nullptr;

		Editor3D::EditorWorldSystem m_WorldSystem;
		Editor3D::TerrainMaterialLibrary m_TerrainMaterialLibrary;

		bool m_TerrainEnabled = true;
		bool m_TerrainPainting = false;
		std::unique_ptr<Onyx::Shader> m_TerrainShader;

		ResolvedModel& ResolveModelCached(const std::string& path, bool checkAnimated = true);

		void CollectLights();
		std::vector<Editor3D::EditorLight> m_CollectedPointLights;
		std::vector<Editor3D::EditorLight> m_CollectedSpotLights;

		Onyx::PostProcessStack m_PostProcessStack;
		uint32_t m_LastPostProcessOutput = 0;
	};

} // namespace MMO
