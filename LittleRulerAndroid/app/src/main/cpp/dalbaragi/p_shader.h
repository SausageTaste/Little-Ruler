#pragma once

#include <string>

#include "p_dalopengl.h"


namespace dal {

	enum class ShaderType { VERTEX, FRAGMENT };

	GLuint compileShader(ShaderType type, const char* const src);


	class ShaderProgram {

	private:
		GLuint mProgramID;
		std::string mName;

	public:
		ShaderProgram(const char* name);

		void attachShader(GLuint shader);

		void link(void);

		void use(void) const;

		GLint getUniformLocation(const char* const identifier) const;
		GLint getAttribLocation(const char* const identifier) const;

	};

}