#pragma once
/*
#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <unordered_map>
#include <set>
#include <memory>

#include "s_logger_god.h"


namespace dal {

    using Entity = std::uint32_t;
    constexpr Entity MAX_NUM_ENTITIES = 512;

    using CpntType = std::uint8_t;
    constexpr CpntType MAX_NUM_CPNTS = 32;
    using CpntSignature = std::bitset<MAX_NUM_CPNTS>;


    class EntityMaster {

    private:
        std::queue<Entity> m_idleEntities;
        std::array<CpntSignature, MAX_NUM_ENTITIES> m_signatures;
        size_t m_livingEntityCount;

    public:
        EntityMaster(void);

        Entity createEntity();
        void destroyEntity(const Entity entity);

        CpntSignature getSignature(const Entity entity) const;
        void setSignature(const Entity entity, const CpntSignature signature);

    };


    class ICpntArray {

    public:
        virtual ~ICpntArray() = default;
        virtual void entityDestroyed(const Entity entity) = 0;

    };


    template<typename T>
    class CpntArray : public ICpntArray {

    private:
        // The packed array of components (of generic type T),
        // set to a specified maximum amount, matching the maximum number
        // of entities allowed to exist simultaneously, so that each entity
        // has a unique spot.
        std::array<T, MAX_NUM_ENTITIES> m_cpntArr;

        std::unordered_map<Entity, size_t> m_entity2Index;
        std::unordered_map<size_t, Entity> m_index2Entity;

        size_t m_size;

    public:
        void insertData(const Entity entity, T&& component) {
            dalAssertm(this->m_entity2Index.find(entity) == this->m_entity2Index.end(), "Component added to same entity more than once.");

            // Put new entry at end and update the maps
            const size_t newIndex = this->m_size;
            this->m_entity2Index[entity] = newIndex;
            this->m_index2Entity[newIndex] = entity;
            this->m_cpntArr[newIndex] = component;
            this->m_size++;
        }

        void removeData(const Entity entity) {
            dalAssertm(this->m_entity2Index.find(entity) != this->m_entity2Index.end(), "Removing non-existent component.");

            // Copy element at end into deleted element's place to maintain density
            const size_t indexOfRemovedEntity = this->m_entity2Index[entity];
            const size_t indexOfLastElement = this->m_size - 1;
            this->m_cpntArr[indexOfRemovedEntity] = m_cpntArr[indexOfLastElement];

            // Update map to point to moved spot
            const Entity entityOfLastElement = this->m_index2Entity[indexOfLastElement];
            this->m_entity2Index[entityOfLastElement] = indexOfRemovedEntity;
            this->m_index2Entity[indexOfRemovedEntity] = entityOfLastElement;

            this->m_entity2Index.erase(entity);
            this->m_index2Entity.erase(indexOfLastElement);

            this->m_size--;
        }

        T& getData(const Entity entity) {
            dalAssertm(this->m_entity2Index.find(entity) != this->m_entity2Index.end(), "Retrieving non-existent component.");

            // Return a reference to the entity's component
            return this->m_cpntArr[this->m_entity2Index[entity]];
        }

        const T& getData(const Entity entity) const {
            dalAssertm(this->m_entity2Index.find(entity) != this->m_entity2Index.end(), "Retrieving non-existent component.");

            // Return a reference to the entity's component
            return this->m_cpntArr[this->m_entity2Index[entity]];
        }

        void entityDestroyed(const Entity entity) override {
            if ( this->m_entity2Index.find(entity) != this->m_entity2Index.end() ) {
                // Remove the entity's component if it existed
                this->removeData(entity);
            }
        }

    };


    class CpntMaster {

    private:
        // Map from type string pointer to a component type
        std::unordered_map<const char*, CpntType> m_cpntTypes;

        // Map from type string pointer to a component array
        std::unordered_map<const char*, std::shared_ptr<ICpntArray>> m_cpntArrays;

        // The component type to be assigned to the next registered component - starting at 0
        CpntType m_nextComponentType = 0;

    public:
        template<typename T>
        void registerComponent(void) {
            const auto typeName = typeid(T).name();

            dalAssertm(this->m_cpntTypes.find(typeName) == this->m_cpntTypes.end(), "Registering component type more than once.");

            // Add this component type to the component type map
            this->m_cpntTypes.insert({ typeName, this->m_nextComponentType });

            // Create a ComponentArray pointer and add it to the component arrays map
            this->m_cpntArrays.insert({ typeName, std::make_shared<CpntArray<T>>() });

            // Increment the value so that the next component registered will be different
            this->m_nextComponentType++;
        }

        template<typename T>
        CpntType getComponentType(void) {
            const auto typeName = typeid(T).name();

            dalAssertm(this->m_cpntTypes.find(typeName) != this->m_cpntTypes.end(), "Component not registered before use.");

            // Return this component's type - used for creating signatures
            return this->m_cpntTypes[typeName];
        }

        template<typename T>
        void addComponent(const Entity entity, T&& component) {
            // Add a component to the array for an entity
            this->getComponentArray<T>()->insertData(entity, component);
        }

        template<typename T>
        void removeComponent(const Entity entity) {
            // Remove a component from the array for an entity
            this->getComponentArray<T>()->removeData(entity);
        }

        template<typename T>
        T& getComponent(const Entity entity) {
            // Get a reference to a component from the array for an entity
            return this->getComponentArray<T>()->getData(entity);
        }

        void entityDestroyed(const Entity entity) {
            // Notify each component array that an entity has been destroyed
            // If it has a component for that entity, it will remove it
            for ( const auto& [type, cpnt] : this->m_cpntArrays ) {
                cpnt->entityDestroyed(entity);
            }
        }

    private:
        // Convenience function to get the statically casted pointer to the ComponentArray of type T.
        template<typename T>
        std::shared_ptr<CpntArray<T>> getComponentArray(void) {
            const char* typeName = typeid(T).name();
            dalAssertm(this->m_cpntTypes.find(typeName) != this->m_cpntTypes.end(), "Component not registered before use.");
            return std::static_pointer_cast<CpntArray<T>>(this->m_cpntArrays[typeName]);
        }

    };


    class ISystem {

    public:
        std::set<Entity> m_entities;

    };


    class SystemMaster {

    private:
        // Map from system type string pointer to a signature
        std::unordered_map<const char*, CpntSignature> m_signatures;

        // Map from system type string pointer to a system pointer
        std::unordered_map<const char*, std::shared_ptr<ISystem>> m_systems;

    public:
        template<typename T>
        std::shared_ptr<T> registerSystem(void) {
            const auto typeName = typeid(T).name();

            dalAssertm(this->m_systems.find(typeName) == this->m_systems.end(), "Registering system more than once.");

            // Create a pointer to the system and return it so it can be used externally
            auto system = std::make_shared<T>();
            this->m_systems.insert({ typeName, system });
            return system;
        }

        template<typename T>
        void setSignature(const CpntSignature signature)
        {
            const auto typeName = typeid(T).name();

            dalAssertm(this->m_systems.find(typeName) != this->m_systems.end(), "System used before registered.");

            // Set the signature for this system
            this->m_signatures.insert({ typeName, signature });
        }

        void entityDestroyed(const Entity entity) {
            // Erase a destroyed entity from all system lists
            // mEntities is a set so no check needed
            for ( auto& [type, system] : this->m_systems ) {
                system->m_entities.erase(entity);
            }
        }

        void entitySignatureChanged(const Entity entity, const CpntSignature entitySignature) {
            // Notify each system that an entity's signature changed
            for ( auto& [type, system] : this->m_systems ) {
                const auto& systemSignature = this->m_signatures[type];

                // Entity signature matches system signature - insert into set
                if ( (entitySignature & systemSignature) == systemSignature ) {
                    system->m_entities.insert(entity);
                }
                // Entity signature does not match system signature - erase from set
                else {
                    system->m_entities.erase(entity);
                }
            }
        }

    };


    class Coordinator {

    private:
        std::unique_ptr<CpntMaster> mComponentManager;
        std::unique_ptr<EntityMaster> mEntityManager;
        std::unique_ptr<SystemMaster> mSystemManager;

    public:
        void init(void) {
            // Create pointers to each manager
            mComponentManager = std::make_unique<CpntMaster>();
            mEntityManager = std::make_unique<EntityMaster>();
            mSystemManager = std::make_unique<SystemMaster>();
        }


        // Entity methods
        Entity createEntity(void) {
            return mEntityManager->createEntity();
        }

        void destroyEntity(const Entity entity) {
            mEntityManager->destroyEntity(entity);

            mComponentManager->entityDestroyed(entity);

            mSystemManager->entityDestroyed(entity);
        }


        // Component methods
        template<typename T>
        void registerComponent() {
            mComponentManager->registerComponent<T>();
        }

        template<typename T>
        void addComponent(const Entity entity, T&& component) {
            mComponentManager->addComponent<T>(entity, component);

            auto signature = mEntityManager->getSignature(entity);
            signature.set(mComponentManager->getComponentType<T>(), true);
            mEntityManager->setSignature(entity, signature);

            mSystemManager->entitySignatureChanged(entity, signature);
        }

        template<typename T>
        void removeComponent(const Entity entity) {
            mComponentManager->removeComponent<T>(entity);

            auto signature = mEntityManager->getSignature(entity);
            signature.set(mComponentManager->getComponentType<T>(), false);
            mEntityManager->setSignature(entity, signature);

            mSystemManager->entitySignatureChanged(entity, signature);
        }

        template<typename T>
        T& getComponent(const Entity entity) {
            return mComponentManager->getComponent<T>(entity);
        }

        template<typename T>
        CpntType getComponentType(void) {
            return mComponentManager->getComponentType<T>();
        }


        // System methods
        template<typename T>
        std::shared_ptr<T> registerSystem() {
            return mSystemManager->registerSystem<T>();
        }

        template<typename T>
        void setSystemSignature(const CpntSignature signature) {
            mSystemManager->setSignature<T>(signature);
        }

    };

}
*/