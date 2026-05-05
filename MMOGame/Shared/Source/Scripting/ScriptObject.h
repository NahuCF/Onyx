#pragma once

#include <string>
#include <string_view>

namespace MMO {

	class ScriptObject
	{
	public:
		explicit ScriptObject(std::string name) : m_Name(std::move(name)) {}
		virtual ~ScriptObject() = default;

		std::string_view GetName() const { return m_Name; }

	private:
		std::string m_Name;
	};

} // namespace MMO
