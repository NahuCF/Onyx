#pragma once

#include "pch.h"

#include "glm.hpp"

namespace se {

	class Shader 
	{
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		~Shader();
		void UseProgramShader();
		void SetUniform(Shader& shader, const char* transformName, glm::mat4& matrix);
		void AddShader(std::vector<Shader*>& contenedor);
		uint32_t m_ProgramID;
	public:
		void MoveRight	(std::vector<Shader*>& contenedor, float x);
		void MoveLeft	(std::vector<Shader*>& contenedor, float x);
		void MoveUp		(std::vector<Shader*>& contenedor, float y);
		void MoveDown	(std::vector<Shader*>& contenedor, float y);

		void MoveShaderAxisY(float y);
		void MoveShaderRight(float x);
		void MoveShaderLeft(float x);

		void SetPos(glm::vec3 vecPos);
		float GetPosX() const { return m_CurrentXPos; }
		float GetPosY() const { return m_CurrentYPos; }	

		float GetRealPosX() const { return m_RealXPos; };
		float GetRealPosY() const { return m_RealYPos; };
	private:
		glm::mat4 m_DefaultPos;
		glm::mat4 m_XAxisMovement;
		glm::mat4 m_YAxisMovement;

		glm::mat4 m_SetPos;
		float m_RealXPos;
		float m_RealYPos;
		float m_CurrentXPos;
		float m_CurrentYPos;

		uint32_t m_VertexID, m_FragmentID;
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

} 