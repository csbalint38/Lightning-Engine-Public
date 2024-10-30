#include "Entity.h"
#include "Script.h"
#include "Transform.h"

#define USE_TRANSFORM_CACHE_MAP 1

namespace lightning::script {
	namespace {
		util::vector<id::generation_type> generations;
		util::deque<script_id> free_ids;

		util::vector<detail::script_ptr> entity_scripts;
		util::vector<id::id_type> id_mapping;

		util::vector<transform::ComponentCache> transform_cache;
		#if USE_TRANSFORM_CACHE_MAP
		std::unordered_map<id::id_type, u32> cache_map;
		#endif

		using script_registry = std::unordered_map<size_t, detail::script_creator>;

		script_registry& registry() {
			static script_registry script_reg;
			return script_reg;
		};

		#ifdef USE_WITH_EDITOR
		util::vector<std::string>& script_names() {
			static util::vector<std::string> names;
			return names;
		}
		#endif

		#if _DEBUG
		bool exists(script_id id) {
			assert(id::is_valid(id));
			const id::id_type index{ id::index(id) };
			assert(index < generations.size() && !(id::is_valid(id_mapping[index]) && id_mapping[index] >= entity_scripts.size()));
			assert(generations[index] == id::generation(id));
			return generations[index] == id::generation(id) && entity_scripts[id_mapping[index]] && entity_scripts[id_mapping[index]]->is_valid();
		}
		#endif

		#if USE_TRANSFORM_CACHE_MAP
		transform::ComponentCache* const get_cache_ptr(const game_entity::Entity* const entity) {
			assert(game_entity::is_alive((*entity).get_id()));
			const transform::transform_id id{ (*entity).transform().get_id() };

			u32 index{ u32_invalid_id };
			auto pair = cache_map.try_emplace(id, id::invalid_id);

			if (pair.second) {
				index = (u32)transform_cache.size();
				transform_cache.emplace_back();
				transform_cache.back().id = id;
				cache_map[id] = index;
			}
			else {
				index = cache_map[id];
			}

			assert(index < transform_cache.size());

			return &transform_cache[index];
		}
		#else
		transform::ComponentCache* const get_cache_ptr(const game_entity::Entity* const entity) {
			assert(game_entity::is_alive((*entity).get_id()));
			const transform::transform_id id{ (*entity).transform().get_id() };

			for (auto& cache : transform_cache) {
				if (cache.id == id) {
					return &cache;
				}
			}

			transform_cache.emplace_back();
			transform_cache.back().id = id;

			return &transform_cache.back();
		}
		#endif
	}

	namespace detail {
		u8 register_script(size_t tag, script_creator func) {
			bool result{ registry().insert(script_registry::value_type{tag, func}).second };
			assert(result);
			return result;
		}

		script_creator get_script_creator_from_engine(size_t tag) {
			auto script = lightning::script::registry().find(tag);
			assert(script != lightning::script::registry().end() && script->first == tag);
			return script->second;
		}

		#ifdef USE_WITH_EDITOR
		u8 add_script_name(const char* name) {
			script_names().emplace_back(name);
			return true;
		}
		#endif
	}

	Component create(InitInfo info, game_entity::Entity entity) {
		assert(entity.is_valid());
		assert(info.script_creator);

		script_id id{};

		if (free_ids.size() > id::min_deleted_elements) {
			id = free_ids.front();
			assert(!exists(id));
			free_ids.pop_back();
			id = script_id{ id::new_generation(id) };
			++generations[id::index(id)];
		}
		else {
			id = script_id{ (id::id_type)id_mapping.size() };
			id_mapping.emplace_back();
			generations.push_back(0);
		}

		assert(id::is_valid(id));
		const id::id_type index{ (id::id_type)entity_scripts.size() };
		entity_scripts.emplace_back(info.script_creator(entity));
		assert(entity_scripts.back()->get_id() == entity.get_id());
		id_mapping[id::index(id)] = index;

		return Component{ id };
	}

	void remove(Component component) {
		assert(component.is_valid() && exists(component.get_id()));
		const script_id id{ component.get_id() };
		const id::id_type index{ id_mapping[id::index(id)] };
		const script_id last_id{ entity_scripts.back()->script().get_id() };
		util::erease_unordered(entity_scripts, index);
		id_mapping[id::index(last_id)] = index;
		id_mapping[id::index(id)] = id::invalid_id;

		if(generations[index] < id::max_generation) {
			free_ids.push_back(id);
		}
	}

	void update(f32 dt) {
		for (const auto& ptr : entity_scripts) {
			ptr->update(dt);
		}

		if (transform_cache.size()) {
			transform::update(transform_cache.data(), (u32)transform_cache.size());
			transform_cache.clear();
			#if USE_TRANSFORM_CACHE_MAP
			cache_map.clear();
			#endif
		}
	}

	void EntityScript::set_rotation(const game_entity::Entity* const entity, math::v4 rotation_quaternion) {
		transform::ComponentCache& cache{ *get_cache_ptr(entity) };
		cache.flags |= transform::ComponentFlags::ROTATION;
		cache.rotation = rotation_quaternion;
	}

	void EntityScript::set_orientation(const game_entity::Entity* const entity, math::v3 orientation_vector) {
		transform::ComponentCache& cache{ *get_cache_ptr(entity) };
		cache.flags |= transform::ComponentFlags::ORIENTATION;
		cache.orientation = orientation_vector;
	}

	void EntityScript::set_position(const game_entity::Entity* const entity, math::v3 position) {
		transform::ComponentCache& cache{ *get_cache_ptr(entity) };
		cache.flags |= transform::ComponentFlags::POSITION;
		cache.position = position;
	}

	void EntityScript::set_scale(const game_entity::Entity* const entity, math::v3 scale) {
		transform::ComponentCache& cache{ *get_cache_ptr(entity) };
		cache.flags |= transform::ComponentFlags::SCALE;
		cache.scale = scale;
	}
}

#ifdef USE_WITH_EDITOR
#include <atlsafe.h>

extern "C" __declspec(dllexport) LPSAFEARRAY get_script_names_from_engine() {
	const u32 size{ (u32)lightning::script::script_names().size() };
	if (!size) return nullptr;
	CComSafeArray<BSTR> names(size);
	for (u32 i{ 0 }; i < size; ++i) {
		names.SetAt(i, A2BSTR_EX(lightning::script::script_names()[i].c_str()), false);
	}
	return names.Detach();
}
#endif
