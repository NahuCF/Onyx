#include "pch.h"

#include "Shader.h"
#include "Source/Maths/glm/glm.hpp"
#include "Source/Maths/glm/gtc/matrix_transform.hpp"
#include "Source/Vendor/GLEW/include/GL/glew.h"

namespace Onyx {

	Shader::Shader(const char* vertexPath, const char* fragmentPath)
	{
		//Open files
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

		//Compile and check for errores
		m_VertexID = glCreateShader(GL_VERTEX_SHADER);
		m_FragmentID = glCreateShader(GL_FRAGMENT_SHADER);
		
		glShaderSource(m_VertexID, 1, &m_VShaderCode, NULL);
		glCompileShader(m_VertexID);

		glGetShaderiv(m_VertexID, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(m_VertexID, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		};

		glShaderSource(m_FragmentID, 1, &m_FShaderCode, NULL);
		glCompileShader(m_FragmentID);

		glGetShaderiv(m_FragmentID, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(m_FragmentID, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FFRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		};

		m_ProgramID = glCreateProgram();
		glAttachShader(m_ProgramID, m_VertexID);
		glAttachShader(m_ProgramID, m_FragmentID);
		glLinkProgram(m_ProgramID);

		glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(m_ProgramID, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}

		glDeleteShader(m_VertexID);
		glDeleteShader(m_FragmentID);
	}

	Shader::~Shader()
	{
		glDeleteProgram(m_ProgramID);
	}

	void Shader::Bind() const
	{
		glUseProgram(m_ProgramID);
	}

	void Shader::UnBind() const
	{
		glUseProgram(0);
	}

	void Shader::SetUniform(Shader& shader, const char* transformName, glm::mat4& matrix)
	{
		glUniformMatrix4fv(glGetUniformLocation(shader.m_ProgramID, transformName), 1, GL_FALSE, &matrix[0][0]);
	}

    void Shader::SetInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(m_ProgramID, name.c_str()), value); 
    }

    void Shader::SetMat4(const std::string &name, glm::mat4& matrix) const
    { 
        glUniformMatrix4fv(glGetUniformLocation(m_ProgramID, name.c_str()), 1, GL_FALSE, &matrix[0][0]);
    }

    void Shader::SetVec3(const std::string &name, glm::vec3& vector) const
    {
        glUniform3fv(glGetUniformLocation(m_ProgramID, name.c_str()), 1, &vector[0]); 
    }

    void Shader::SetVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(m_ProgramID, name.c_str()), x, y, z); 
    }

    void Shader::SetFloat(const std::string &name, float value) const
    { 
        glUniform1f(glGetUniformLocation(m_ProgramID, name.c_str()), value); 
    }
} 