#pragma once
#include "ComponentsCommonHeaders.h"

namespace lightning::script {

    struct InitInfo
    {
        detail::script_creator script_creator;
    };

    Component create(InitInfo info, game_entity::Entity entity);
    void remove(Component component);
    void update(f32 dt);
}