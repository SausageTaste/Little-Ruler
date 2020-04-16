#include "d_mapdata.h"


namespace dal::v1 {

    size_t Mesh::numVertices(void) const {
        assert(this->checkSizeValidity());
        return this->m_uvcoords.size() / 2;
    }

    bool Mesh::checkSizeValidity(void) const {
        if ( this->m_vertices.size() != this->m_normals.size() )
            return false;
        if ( 2 * this->m_vertices.size() != 3 * this->m_uvcoords.size() )
            return false;

        return true;
    }

}
