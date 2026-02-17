#include "pch.h"

#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

namespace Onyx {

	Shader::Shader(const char* vertexPath, const char* fragmentPath)
	{
		m_VShaderFile.open(vertexPath);
		m_FShaderFile.open(fragmentPath);

		if (!m_VShaderFile.is_open()) {
			std::cout << "ERROR::SHADER::VERTEX::FILE_NOT_FOUND: " << vertexPath << std::endl;
		}
		if (!m_FShaderFile.is_open()) {
			std::cout << "ERROR::SHADER::FRAGMENT::FILE_NOT_FOUND: " << fragmentPath << std::endl;
		}

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

    int Shader::GetLocation(const std::string& name)
    {
        auto it = m_UniformLocationCache.find(name);
        if (it != m_UniformLocationCache.end()) return it->second;

        int location = glGetUniformLocation(m_ProgramID, name.c_str());
        m_UniformLocationCache[name] = location;
        return location;
    }

	void Shader::SetUniform(Shader& shader, const char* transformName, const glm::mat4& matrix)
	{
		glUniformMatrix4fv(glGetUniformLocation(shader.m_ProgramID, transformName), 1, GL_FALSE, &matrix[0][0]);
	}

    void Shader::SetInt(const std::string &name, int value)
    {
        glUniform1i(GetLocation(name), value);
    }

    void Shader::SetMat4(const std::string &name, const glm::mat4& matrix)
    {
        glUniformMatrix4fv(GetLocation(name), 1, GL_FALSE, &matrix[0][0]);
    }

    void Shader::SetMat4(int location, const glm::mat4& matrix)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
    }

    void Shader::SetVec2(const std::string &name, float x, float y)
    {
        glUniform2f(GetLocation(name), x, y);
    }

    void Shader::SetVec3(const std::string &name, const glm::vec3& vector)
    {
        glUniform3fv(GetLocation(name), 1, &vector[0]);
    }

    void Shader::SetVec3(const std::string &name, float x, float y, float z)
    {
        glUniform3f(GetLocation(name), x, y, z);
    }

    void Shader::SetVec4(const std::string &name, float x, float y, float z, float w)
    {
        glUniform4f(GetLocation(name), x, y, z, w);
    }

    void Shader::SetFloat(const std::string &name, float value)
    {
        glUniform1f(GetLocation(name), value);
    }

    void Shader::SetIntArray(const std::string &name, const int* values, int count)
    {
        glUniform1iv(GetLocation(name), count, values);
    }

    void Shader::SetFloatArray(const std::string &name, const float* values, int count)
    {
        glUniform1fv(GetLocation(name), count, values);
    }
}
