#pragma once
#include "ComponentsCommonHeaders.h"

namespace lightning::geometry {
  struct InitInfo {
    id::id_type geometry_content_id;
    u32 material_count;
    id::id_type*  material_ids;
  };

  Component create(InitInfo info, game_entity::Entity entity);
  void remove(Component c);
  void get_render_item_ids(id::id_type* const item_ids, u32 count);
}