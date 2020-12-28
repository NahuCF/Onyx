#include <GL/glew.h>
#include <iostream>

#include "shader.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

namespace se {namespace graphics {


	Shader::Shader(const char* vertexPath, const char* fragmentPath)
		: m_ActualXPos(0), m_ActualYPos(0)
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
		if (!success)
		{
			glGetShaderInfoLog(m_VertexID, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		};

		glShaderSource(m_FragmentID, 1, &m_FShaderCode, NULL);
		glCompileShader(m_FragmentID);

		glGetShaderiv(m_FragmentID, GL_COMPILE_STATUS, &success);
		if (!success)
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

		//Initializating variable
		UseProgramShader();
		m_DefaultPos = glm::mat4(1.0f);
		m_DefaultPos = glm::translate(m_DefaultPos, glm::vec3(0.0f, -0.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(this->m_ProgramID, "move"), 1, GL_FALSE, &m_DefaultPos[0][0]);
	}

	void Shader::UseProgramShader()
	{
		glUseProgram(this->m_ProgramID);
	}

	void Shader::SetUniform(Shader& shader, const char* transformName, glm::mat4& matrix)
	{
		glUniformMatrix4fv(glGetUniformLocation(shader.m_ProgramID, transformName), 1, GL_FALSE, &matrix[0][0]);
	}

	void Shader::Añadir(Shader* contenedor[], int index)
	{
		contenedor[index] = this;
	}

	void Shader::MoveRight(Shader* contenedor[], float x)
	{
		for(int i = 0; i < sizeof(contenedor) / sizeof(Shader); i++)
		{
			contenedor[i]->UseProgramShader();
			m_ActualXPos -= x;
			m_XAxisMovement = glm::mat4(1.0f);
			m_XAxisMovement = glm::translate(m_XAxisMovement, glm::vec3(m_ActualXPos, m_ActualYPos, 0.0f));
			glUniformMatrix4fv(glGetUniformLocation(m_ProgramID, "move"), 1, GL_FALSE, &m_XAxisMovement[0][0]);
		}
	}
	void Shader::MoveLeft(Shader* contenedor[], float x)
	{
		for (int i = 0; i < sizeof(contenedor) / sizeof(Shader); i++)
		{
			contenedor[i]->UseProgramShader();
			contenedor[i]->m_ActualXPos += x;
			contenedor[i]->m_XAxisMovement = glm::mat4(1.0f);
			contenedor[i]->m_XAxisMovement = glm::translate(contenedor[0]->m_XAxisMovement, glm::vec3(contenedor[0]->m_ActualXPos, contenedor[0]->m_ActualYPos, 0.0f));
			glUniformMatrix4fv(glGetUniformLocation(contenedor[0]->m_ProgramID, "move"), 1, GL_FALSE, &contenedor[0]->m_XAxisMovement[0][0]);
		}
	}
	void Shader::MoveUp(Shader* contenedor[], float y)
	{
		for (int i = 0; i < sizeof(contenedor) / sizeof(Shader); i++)
		{
			contenedor[i]->UseProgramShader();
			contenedor[i]->m_ActualYPos -= y;
			contenedor[i]->m_YAxisMovement = glm::mat4(1.0f);
			contenedor[i]->m_YAxisMovement = glm::translate(contenedor[i]->m_YAxisMovement, glm::vec3(contenedor[i]->m_ActualXPos, contenedor[i]->m_ActualYPos, 0.0f));
			glUniformMatrix4fv(glGetUniformLocation(contenedor[i]->m_ProgramID, "move"), 1, GL_FALSE, &contenedor[i]->m_YAxisMovement[0][0]);
		}
	}
	void Shader::MoveDown(Shader* contenedor[], float y)
	{
		for (int i = 0; i < sizeof(contenedor) / sizeof(Shader); i++)
		{
			contenedor[i]->UseProgramShader();
			contenedor[i]->m_ActualYPos += y;
			contenedor[i]->m_YAxisMovement = glm::mat4(1.0f);
			contenedor[i]->m_YAxisMovement = glm::translate(contenedor[i]->m_YAxisMovement, glm::vec3(contenedor[i]->m_ActualXPos, contenedor[i]->m_ActualYPos, 0.0f));
			glUniformMatrix4fv(glGetUniformLocation(contenedor[i]->m_ProgramID, "move"), 1, GL_FALSE, &contenedor[i]->m_YAxisMovement[0][0]);
		}
	}
	
	void Shader::SetPos(glm::vec3 vecPos)
	{
		m_ActualYPos = vecPos.y;
		m_ActualXPos = vecPos.x;
		m_SetPos = glm::mat4(1.0f);
		m_SetPos = glm::translate(m_SetPos, vecPos);
		glUniformMatrix4fv(glGetUniformLocation(this->m_ProgramID, "move"), 1, GL_FALSE, &m_SetPos[0][0]);
	}

} }