#pragma once

#include "pch.h"

#include <glm/glm.hpp>
#include <unordered_map>

namespace Onyx {

	class Shader 
	{
	public:
		Shader(const char* vertexPath, const char* fragmentPath);
		~Shader();

		void Bind() const;
		void UnBind() const;

		uint32_t GetProgramID() const { return m_ProgramID; }

		void SetUniform(Shader& shader, const char* transformName, const glm::mat4& matrix);
        void SetInt(const std::string &name, int value);
        void SetMat4(const std::string &name, const glm::mat4& matrix);
        void SetMat4(int location, const glm::mat4& matrix);
        void SetVec2(const std::string &name, float x, float y);
        void SetVec3(const std::string &name, const glm::vec3& vector);
        void SetVec3(const std::string &name, float x, float y, float z);
        void SetVec4(const std::string &name, float x, float y, float z, float w);
        void SetFloat(const std::string &name, float value);
        void SetIntArray(const std::string &name, const int* values, int count);
        void SetFloatArray(const std::string &name, const float* values, int count);

        int GetLocation(const std::string& name);
	private:

		uint32_t m_ProgramID;
        std::unordered_map<std::string, int> m_UniformLocationCache;
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