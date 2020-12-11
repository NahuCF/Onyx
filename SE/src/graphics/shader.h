#pragma once

#include <fstream>
#include <sstream>
#include <string>

#include "glm.hpp"

namespace se { namespace graphics {

	class Shader 
	{
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		void UseProgramShader();
		void SetUniform(Shader& shader, const char* transformName, glm::mat4& matrix);
		void MoveTo(glm::vec3 vectorPos);
		unsigned int m_ProgramID;
	private:
		glm::mat4 m_Mat2Base;
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