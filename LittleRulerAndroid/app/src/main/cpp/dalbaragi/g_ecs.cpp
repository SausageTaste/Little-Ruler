#include "g_ecs.h"
/*
#include "s_logger_god.h"


namespace dal {

    EntityMaster::EntityMaster(void)
        : m_livingEntityCount(0)
    {
        for ( Entity entity = 0; entity < MAX_NUM_ENTITIES; ++entity ) {
            this->m_idleEntities.push(entity);
        }
    }

    Entity EntityMaster::createEntity() {
        dalAssertm(this->m_livingEntityCount < MAX_NUM_ENTITIES, "Too many entities in existence.");

        Entity id = this->m_idleEntities.front();
        this->m_idleEntities.pop();
        ++this->m_livingEntityCount;

        dalAssert(this->m_livingEntityCount == (MAX_NUM_ENTITIES - this->m_idleEntities.size()));

        return id;
    }

    void EntityMaster::destroyEntity(const Entity entity) {
        dalAssertm(entity < MAX_NUM_ENTITIES, "Entity out of range.");
        this->m_signatures[entity].reset();
        this->m_idleEntities.push(entity);
        --this->m_livingEntityCount;

        dalAssert(this->m_livingEntityCount == (MAX_NUM_ENTITIES - this->m_idleEntities.size()));
    }

    void EntityMaster::setSignature(const Entity entity, const CpntSignature signature) {
        dalAssertm(entity < MAX_NUM_ENTITIES, "Entity out of range.");

        this->m_signatures[entity] = signature;
    }

    CpntSignature EntityMaster::getSignature(const Entity entity) const {
        dalAssertm(entity < MAX_NUM_ENTITIES, "Entity out of range.");

        return this->m_signatures[entity];
    }

}
*/