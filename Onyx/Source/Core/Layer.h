#pragma once

namespace Onyx {

	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnUpdate() = 0;
		virtual void OnImGui() {}
		virtual void OnAttach() {}
	};

} // namespace Onyx