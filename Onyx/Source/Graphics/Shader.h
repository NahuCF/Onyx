#pragma once

#include "pch.h"

#include "Vendor/glm/glm/glm.hpp"

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
        void SetMat4(const std::string &name, glm::mat4& matrix) const;
        void SetVec3(const std::string &name, glm::vec3& vector) const;
        void SetVec3(const std::string &name, float x, float y, float z) const;
        void SetFloat(const std::string &name, float value) const;
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