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
		void Añadir(Shader* contenedor[], int index);
		unsigned int m_ProgramID;
	public:
		void MoveRight	(Shader* contenedor[], float x, unsigned int arrayLength);
		void MoveLeft	(Shader* contenedor[], float x, unsigned int arrayLength);
		void MoveUp		(Shader* contenedor[], float y, unsigned int arrayLength);
		void MoveDown	(Shader* contenedor[], float y, unsigned int arrayLength);

		void SetPos(glm::vec3 vecPos);
		float GetPosX();
		float GetPosY();
	private:
		glm::mat4 m_DefaultPos;
		glm::mat4 m_XAxisMovement;
		glm::mat4 m_YAxisMovement;
		float m_ActualXPos;
		float m_ActualYPos;

		glm::mat4 m_SetPos;
		float m_CurrentXPos;
		float m_CurrentYPos;

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