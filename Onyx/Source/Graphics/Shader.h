#pragma once

#include "pch.h"

#include "glm.hpp"

namespace Onyx {

	class Shader 
	{
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		~Shader();

		void Bind() const;
		void UnBind() const;

		uint32_t GetProgramID() const { return m_ProgramID; }

		void SetUniform(Shader& shader, const char* transformName, glm::mat4& matrix);
        void SetInt(const std::string &name, int value) const;
	private:
		uint32_t m_ProgramID;
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