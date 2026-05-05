#pragma once

#include "Source/Core/Base.h"
#include "Source/Core/ImGuiLayer.h"
#include "Source/Core/Layer.h"
#include "Source/Graphics/AssetManager.h"
#include "Source/Graphics/Window.h"

#include <memory>
#include <string>
#include <vector>

int main();

namespace Onyx {

	struct ApplicationSpec
	{
		uint32_t windowWidth = 1280;
		uint32_t windowHeight = 720;
		std::string applicationName = "Application";
	};

	class Application
	{
	public:
		Application(ApplicationSpec& spec);
		virtual ~Application();

		void PushLayer(Layer* layer);
		Ref<Onyx::Window> GetWindow() { return m_Window; }
		AssetManager& GetAssetManager() { return *m_AssetManager; }
		static Application& GetInstance() { return *s_Instance; }
	private:
		void Run();

	private:
		bool m_IsRunning = true;
		std::vector<Layer*> m_LayerStack;
		Ref<Onyx::Window> m_Window;
		Onyx::ImGuiLayer* m_ImGuiLayer;
		std::unique_ptr<AssetManager> m_AssetManager;
		static Application* s_Instance;

		friend int ::main();
	};

	Application* CreateApplication();

} // namespace Onyx