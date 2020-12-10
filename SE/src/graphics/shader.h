#pragma once

#include <fstream>
#include <sstream>
#include <string>

namespace se { namespace graphics {

	class Shader 
	{
	public:
		void UseProgramShader();
		Shader(const char* vertexPath, const char* fragmentPath);
		unsigned int m_ProgramID;
	private:
		unsigned int m_VertexID, m_FragmentID;
		const char* m_VShaderCode;
		const char* m_FShaderCode;
		int success;
		char infoLog[512];
		std::string m_VertexCode;
		std::string m_FragmentCode;
		std::ifstream m_VShaderFile;
		std::ifstream m_FShaderFile;
		std::stringstream m_VShaderStream, m_FShaderStream;
	};

} }