#pragma once

#include <string>

#include "p_dalopengl.h"


namespace dal {

	class MeshStatic {
		//////// Variables ////////

	private:
		std::string mName;

		GLuint mVao;

		GLuint mVertexArrayBuffer;
		GLuint mTexCoordArrayBuffer;
		GLuint mNormalArrayBuffe;

		unsigned int mVertexSize;

		//////// Functions ////////

	public:
		MeshStatic(void);
		~MeshStatic(void);

		MeshStatic(const MeshStatic&);
		MeshStatic& operator=(const MeshStatic&);

		void draw(void) const;

		int buildData(
			const float* const vertices, const int vertSize,
			const float* const texcors, const int texcorSize,
			const float* const normals, const int norSize
		);
		void destroyData(void);
		bool isReady(void) const;

		void setName(const char* const name);

	private:
		void createBuffers(void);

		void bindVAO(void) const;
		static void unbindVAO(void);

	};

}