// Entity.h

#pragma once
#include "ComponentsCommonHeaders.h"

namespace lightning {
    #define INIT_INFO(component) namespace component { struct InitInfo; }
  
    INIT_INFO(transform);
    INIT_INFO(script);
    INIT_INFO(geometry);

    #undef INIT_INFO

    namespace game_entity {
        struct EntityInfo
        {
        transform::InitInfo* transform{ nullptr };
        script::InitInfo* script{ nullptr };
        geometry::InitInfo* geometry{ nullptr };
        };

        Entity create(EntityInfo info);
        void remove(entity_id id);
        bool is_alive(entity_id id);
    }
}
