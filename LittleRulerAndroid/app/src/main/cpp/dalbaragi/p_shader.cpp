#include "p_shader.h"

#include <string>

#include "s_logger_god.h"


using namespace std::string_literals;


namespace dal {

	GLuint compileShader(ShaderType type, const char* const src) {
		// Returns 0 on error
		GLuint shaderID = 0;

		switch (type)
		{
		case ShaderType::VERTEX:
			shaderID = glCreateShader(GL_VERTEX_SHADER);
			break;
		case ShaderType::FRAGMENT:
			shaderID = glCreateShader(GL_FRAGMENT_SHADER);
			break;
		}

		if (shaderID == 0) {
			LoggerGod::getinst().putError("Failed to create shader object.");
			throw -1;
		}

#if defined(_WIN32)
		const std::string srcStr{ "#version 330 core\n"s + src };
		const GLchar* sourceCstr[] = { srcStr.c_str() };
#elif defined(__ANDROID__)
		const std::string srcStr{ "#version 300 es\n"s + src };
		const GLchar* sourceCstr[] = { srcStr.c_str() };
#endif

		glShaderSource(shaderID, 1, sourceCstr, NULL);
		glCompileShader(shaderID);

		GLint vShaderCompiled = GL_FALSE;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &vShaderCompiled);
		if (vShaderCompiled != GL_TRUE) {
			GLsizei length = 0;
			char log[1024];
			glGetShaderInfoLog(shaderID, 1024, &length, log);
			LoggerGod::getinst().putFatal(
				"ShaderPrimitive compile error. Error message from OpenGL will follow.\n"s + log
			);
			LoggerGod::getinst().putFatal("And shader source code is following.\n"s + srcStr);
			throw -1;
		}

		return shaderID;
	}

}


namespace dal {

	ShaderProgram::ShaderProgram(const char* name)
		: mProgramID(glCreateProgram()),
		mName(name)
	{

	}

	void ShaderProgram::attachShader(GLuint shader) {
		glAttachShader(this->mProgramID, shader);
	}

	void ShaderProgram::link(void) {
		glLinkProgram(this->mProgramID);

		GLint programSuccess = GL_TRUE;
		glGetProgramiv(this->mProgramID, GL_LINK_STATUS, &programSuccess);
		if (programSuccess != GL_TRUE) {
			GLsizei length = 0;
			char log[100];
			glGetProgramInfoLog(this->mProgramID, 100, &length, log);
			auto errMsg = "ShaderProgram linking error: "s + log;
			LoggerGod::getinst().putFatal(errMsg);
			throw -1;
		}
	}

	void ShaderProgram::use(void) const {
		glUseProgram(this->mProgramID);
	}

	GLint ShaderProgram::getUniformLocation(const char* const identifier) const {
		auto id = glGetUniformLocation(this->mProgramID, identifier);
		if (id < 0) {
			LoggerGod::getinst().putWarn("Uniform "s + identifier + " not found in shader "s + mName);
		}
		return id;
	}

	GLint ShaderProgram::getAttribLocation(const char* const identifier) const {
		auto id = glGetAttribLocation(this->mProgramID, identifier);
		if (id < 0) {
			LoggerGod::getinst().putWarn("Attrib "s + identifier + " not found in shader "s + mName);
		}
		return id;
	}

}