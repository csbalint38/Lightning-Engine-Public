#include "Geometry.h"
#include "Entity.h"
#include "Graphics/Renderer.h"

namespace lightning::geometry {
  namespace {
    util::vector<u32> active_lod;
    util::vector<id::id_type> render_item_ids;
    util::vector<geometry_id> owner_ids;
    util::vector<id::id_type> id_mapping;

    util::vector<id::generation_type> generations;
    util::deque<geometry_id> free_ids;

    #if _DEBUG
    bool exists(geometry_id id) {
      assert(id::is_valid(id));
      const id::id_type index{ id::index(id) };
      assert(index < generations.size() && !(id::is_valid(id_mapping[index]) && id_mapping[index] >= render_item_ids.size()));
      assert(generations[index] == id::generation(id));

      return (generations[index] == id::generation(id)) && id::is_valid(id_mapping[index]) && id::is_valid(render_item_ids[id_mapping[index]]);
    }
    #endif
  }

  Component create(InitInfo info, game_entity::Entity entity) {
    assert(entity.is_valid());
    assert(id::is_valid(info.geometry_content_id) && info.material_count && info.material_ids);

    geometry_id id{};

    if(free_ids.size() > id::min_deleted_elements) {
      id = free_ids.front();
      assert(!exists(id));
      free_ids.pop_front();
      id = geometry_id{ id::new_generation(id) };
      ++generations[id::index(id)];
    }
    else {
      id = geometry_id{ (id::id_type)id_mapping.size() };
      id_mapping.emplace_back();
      generations.push_back(0);
    }

    assert(id::is_valid(id));
    const id::id_type index{ (id::id_type)render_item_ids.size() };
    active_lod.emplace_back(0);
    render_item_ids.emplace_back(graphics::add_render_item(entity.get_id(), info.geometry_content_id, info.material_count, info.material_ids));
    owner_ids.emplace_back(id::index(id));
    id_mapping[id::index(id)] = index;

    return Component{ id };
  }

  void remove(Component c) {
    assert(c.is_valid() && exists(c.get_id()));
    const geometry_id id{ c.get_id() };
    const id::id_type index{ id_mapping[id::index(id)] };
    const geometry_id last_id{ owner_ids.back() };
    graphics::remove_render_item(render_item_ids[index]);
    util::erease_unordered(active_lod, index);
    util::erease_unordered(render_item_ids, index);
    util::erease_unordered(owner_ids, index);
    id_mapping[id::index(last_id)] = index;
    id_mapping[id::index(id)] = id::invalid_id;

    if(generations[index] < id::max_generation) {
      free_ids.push_back(id);
    }
  }

  void get_render_item_ids(id::id_type* const item_ids, u32 count) {
    assert(render_item_ids.size() >= count);
    memcpy(item_ids, render_item_ids.data(), count * sizeof(id::id_type));
  }
}