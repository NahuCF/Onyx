#include <fstream>
#include <sstream>
#include <string>

#include <GL/glew.h>
#include "shader.h"

namespace se {namespace graphics {

	Shader::Shader(const char* vertexPath, const char* fragmentPath)
	{

		m_VShaderFile.open(vertexPath);
		m_FShaderFile.open(fragmentPath);

		m_VShaderStream << m_VShaderFile.rdbuf();
		m_FShaderStream << m_FShaderFile.rdbuf();

		m_VShaderFile.close();
		m_FShaderFile.close();

		m_VertexCode = m_VShaderStream.str();
		m_FragmentCode = m_FShaderStream.str();

		m_VShaderCode = m_VertexCode.c_str();
		m_FShaderCode = m_FragmentCode.c_str();

		m_VertexID = glCreateShader(GL_VERTEX_SHADER);
		m_FragmentID = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(m_VertexID, 1, &m_VShaderCode, NULL);
		glShaderSource(m_FragmentID, 1, &m_FShaderCode, NULL);

		m_ProgramID = glCreateProgram();
		glAttachShader(m_ProgramID, m_VertexID);
		glAttachShader(m_ProgramID, m_FragmentID);
		glLinkProgram(m_ProgramID);

		glDeleteShader(m_VertexID);
		glDeleteShader(m_FragmentID);

	}

	void Shader::UseProgramShader(unsigned int ID)
	{
		glUseProgram(m_ProgramID);
	}

} }