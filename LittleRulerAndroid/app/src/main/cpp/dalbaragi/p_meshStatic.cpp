#include "p_meshStatic.h"

#include <string>

#include "s_logger_god.h"


using namespace std::string_literals;


namespace dal {

	void MeshStatic::draw(void) const {
#ifdef _DEBUG
		if (!this->isReady())
			LoggerGod::getinst().putError("MeshStatic::renderDepthmap called without being built.");
#endif

		this->bindVAO();
		glDrawArrays(GL_TRIANGLES, 0, this->mVertexSize);
		this->unbindVAO();
	}

	int MeshStatic::buildData(
		const float* const vertices, const int vertSize,
		const float* const texcors, const int texcorSize,
		const float* const normals, const int norSize
	) {
		/* Check if data is wrong. */ {
			if (this->isReady()) {
				LoggerGod::getinst().putError("MeshStatic's data already built.");
				return -1;
			}

			const auto numOfVertex = vertSize / 3;

			if (numOfVertex != (texcorSize / 2)) {
				LoggerGod::getinst().putError(std::string("\'texCoords\' have different number of vertices: " + std::to_string(vertSize) + ", " + std::to_string(texcorSize)));
				return -1;
			}

			if (numOfVertex != (norSize / 3)) {
				LoggerGod::getinst().putError("\'normals\' have different number of vertices: "s + std::to_string(vertSize) + ", "s + std::to_string(norSize));
				return -1;
			}
		}

		this->createBuffers();
		this->bindVAO();
		size_t  vramUsage = 0;

		/* Vertices */ {
			auto size = vertSize * sizeof(float);

			vramUsage += size;

			glBindBuffer(GL_ARRAY_BUFFER, this->mVertexArrayBuffer);
			glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		}

		/* TexCoords */ {
			auto size = texcorSize * sizeof(float);
			vramUsage += size;

			glBindBuffer(GL_ARRAY_BUFFER, this->mTexCoordArrayBuffer);
			glBufferData(GL_ARRAY_BUFFER, size, texcors, GL_STATIC_DRAW);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		}

		/* Normals */ {
			auto size = norSize * sizeof(float);
			vramUsage += size;

			glBindBuffer(GL_ARRAY_BUFFER, this->mNormalArrayBuffe);
			glBufferData(GL_ARRAY_BUFFER, size, normals, GL_STATIC_DRAW);

			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(2);
		}

		/* Finish */ {
			this->unbindVAO();
			this->mVertexSize = vertSize / 3;

			//LoggerGod::getinst().putInfo("created MeshStatic with "s + to_string(this->mVertexSize) + " vertices."s);
		}

		return 0;
	}

	void MeshStatic::destroyData(void) {
		GLuint bufferIds[3] = {
			this->mVertexArrayBuffer,
			this->mTexCoordArrayBuffer,
			this->mNormalArrayBuffe,
		};
		glDeleteBuffers(3, bufferIds);

		this->mVertexArrayBuffer = 0;
		this->mTexCoordArrayBuffer = 0;
		this->mNormalArrayBuffe = 0;

		glDeleteVertexArrays(1, &this->mVao);
		this->mVao = 0;

		this->mVertexSize = 0;

		LoggerGod::getinst().putInfo("destroyed MeshStatic with "s + std::to_string(this->mVertexSize) + " vertices."s);
	}

	bool MeshStatic::isReady(void) const {
		return mVertexSize != 0;
	}

	void MeshStatic::setName(const char* const name) {
		mName = name;
	}

	//// Private ////

	void MeshStatic::createBuffers(void) {
		glGenVertexArrays(1, &this->mVao);
		if (this->mVao <= 0) {
			LoggerGod::getinst().putFatal("Failed to generate vertex array.");
			throw -1;
		}

		GLuint bufferIds[3];
		glGenBuffers(3, bufferIds);

		for (int i = 0; i < 3; i++) {
			if (bufferIds[i] == 0) {
				LoggerGod::getinst().putFatal("Failed to generate beffer.");
				throw -1;
			}
		}

		this->mVertexArrayBuffer = bufferIds[0];
		this->mTexCoordArrayBuffer = bufferIds[1];
		this->mNormalArrayBuffe = bufferIds[2];
	}

	void MeshStatic::bindVAO(void) const {
		glBindVertexArray(this->mVao);
	}

	void MeshStatic::unbindVAO(void) {
		glBindVertexArray(0);
	}

}