#pragma once

#include <ostream>

namespace se {

	class String
	{
	public:
		String(const char* text);
	private:
		const char* m_TextContent;
		friend std::ostream& operator<<(std::ostream& stream, se::String& text);
	};

}