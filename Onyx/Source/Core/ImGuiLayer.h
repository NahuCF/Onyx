#pragma once

#include "Source/Core/Layer.h"

namespace Onyx {

	class ImGuiLayer : public Layer
	{
	public:
		void OnAttach() override;
		void OnUpdate() {}
		void OnImGui() override;

		void Begin();
		void End();
	};

}
