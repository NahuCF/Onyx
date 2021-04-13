#include "pch.h"
#include "Source/DS/String.h"

namespace se {

	String::String(const char* text)
		: m_TextContent(text)
	{
		m_SizeString = strlen(text);
	}

	std::ostream& operator<<(std::ostream& stream, const se::String& text)
	{
		stream << text.m_TextContent;
		return stream;
	}

}