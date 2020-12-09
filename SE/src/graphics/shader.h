#pragma once

namespace se { namespace graphics {

	class Shader 
	{
	public:
		void UseProgramShader(unsigned int ID);
	private:
		Shader(const char* vertexPath, const char* fragmentPath);
	private:
		unsigned int m_ProgramID, m_VertexID, m_FragmentID;
		const char* m_VShaderCode;
		const char* m_FShaderCode;
		std::string m_VertexCode;
		std::string m_FragmentCode;
		std::ifstream m_VShaderFile;
		std::ifstream m_FShaderFile;
		std::stringstream m_VShaderStream, m_FShaderStream;
	};

} }