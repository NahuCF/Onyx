#include "pch.h"

#include "Source/Core/Application.h"
#include "Source/Core/Base.h"
#include "Source/Core/ImGuiLayer.h"
#include "Source/UI/Manager.h"
#include "Source/UI/UILayer.h"

namespace Onyx {

	Application* Application::s_Instance = nullptr;

	Application::Application(ApplicationSpec& spec)
	{
		m_Window = Onyx::MakeRef<Onyx::Window>(spec.applicationName.c_str(), spec.windowWidth, spec.windowHeight);

		s_Instance = this;

		m_AssetManager = std::make_unique<AssetManager>();
		m_WorldUIAnchorSystem = std::make_unique<WorldUIAnchorSystem>();

		m_ImGuiLayer = new ImGuiLayer();
		PushLayer(m_ImGuiLayer);

		// UI subsystem. Manager is owned by Application; UILayer ticks it
		// each frame between user layers' OnUpdate (3D rendering) and
		// ImGui's render. UILayer is NOT in m_LayerStack so its execution
		// order is fixed regardless of user PushLayer calls.
		m_UI = std::make_unique<Onyx::UI::Manager>();
		m_UILayer = std::make_unique<Onyx::UI::UILayer>(*m_UI, *m_Window);
		m_UILayer->OnAttach();
	}

	Application::~Application()
	{
		for (Layer* layer : m_LayerStack)
			delete layer;
	}

	void Application::Run()
	{
		while (!m_Window->ShouldClose())
		{
			m_Window->Clear();

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			// UI ticks after user layers (3D rendering complete) but before
			// ImGui — ImGui draws over top so editor panels stay accessible.
			m_UILayer->OnUpdate();

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGui();
			m_UILayer->OnImGui();
			m_ImGuiLayer->End();

			m_Window->Update();
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.push_back(layer);
		layer->OnAttach();
	}

} // namespace Onyx