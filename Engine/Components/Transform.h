#pragma once
#include "ComponentsCommonHeaders.h"

namespace lightning::transform {

    struct InitInfo
    {
        f32 position[3]{};
        f32 rotation[4]{};
        f32 scale[3]{1.f, 1.f, 1.f};
    };

    struct ComponentFlags {
        enum Flags : u32 {
            ROTATION = 0x01,
            ORIENTATION = 0x02,
            POSITION = 0x04,
            SCALE = 0x08,
            ALL = ROTATION | ORIENTATION | POSITION | SCALE
        };
    };

    struct ComponentCache {
        math::v4 rotation;
        math::v3 orientation;
        math::v3 position;
        math::v3 scale;
        transform_id id;
        u32 flags;
    };

    Component create(InitInfo info, game_entity::Entity entity);
    void remove(Component component);
    void get_transform_matrices(const game_entity::entity_id id, math::m4x4& world, math::m4x4& inverse_world);
    void get_updated_component_flags(const game_entity::entity_id* const ids, u32 count, u8* const flags);
    void update(const ComponentCache* const cache, u32 count);
}