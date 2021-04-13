#pragma once

//#include <ostream>

namespace se {

	class String
	{
	public:
		String(const char* text);
	private:
		const char* m_TextContent;
		char* m_NewTextContent;
		int m_SizeString;
		int m_NewSizeString;

		friend std::ostream& operator<<(std::ostream& stream, const se::String& text);
		char* operator+(char* text);
	};

}