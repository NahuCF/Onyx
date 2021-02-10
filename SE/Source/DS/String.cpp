#include "pch.h"
#include "Source/DS/String.h"

namespace se {

	String::String(const char* text)
		: m_TextContent(text)
	{
	}

	std::ostream& operator<<(std::ostream& stream, se::String& text)
	{
		stream << text.m_TextContent;
		return stream;
	}

}