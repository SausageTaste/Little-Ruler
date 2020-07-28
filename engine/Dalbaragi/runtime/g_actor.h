#pragma once

#include "p_animation.h"
#include "d_collider.h"


// Actor
namespace dal {

    class ActorInfo {

    public:
        enum class ColliderType {
            aabb = 0,
            none = 1,
            mesh = 2,
        };

    public:
        Transform m_transform;
        std::string m_name;
        std::vector<std::int32_t> m_envmapIndices;
        ColliderType m_colType = ColliderType::aabb;

    public:
        ActorInfo(void) = default;
        ActorInfo(const std::string& actorName);

    };

}


// Components
namespace dal::cpnt {

    constexpr unsigned int MAX_ID_NAME_LEN = 128;


    using Transform = dal::Transform;

    struct Identifier {
        char m_name[MAX_ID_NAME_LEN] = { 0 };
    };

    struct PhysicsObj {
        bool m_touchingGround = false;
    };

}
