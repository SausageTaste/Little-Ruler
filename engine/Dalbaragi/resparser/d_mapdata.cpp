#include "d_mapdata.h"


namespace dal::v2 {

    size_t MeshRaw::numVertices(void) const {
        assert(this->checkSizeValidity());
        return this->m_uvcoords.size() / 2;
    }

    const float* MeshRaw::vertices(void) const {
        assert(this->checkSizeValidity());
        return this->m_vertices.data();
    }

    const float* MeshRaw::uvcoords(void) const {
        assert(this->checkSizeValidity());
        return this->m_uvcoords.data();
    }

    const float* MeshRaw::normals(void) const {
        assert(this->checkSizeValidity());
        return this->m_normals.data();
    }

    bool MeshRaw::checkSizeValidity(void) const {
        if ( this->m_vertices.size() != this->m_normals.size() )
            return false;
        if ( 2 * this->m_vertices.size() != 3 * this->m_uvcoords.size() )
            return false;

        return true;
    }

}
