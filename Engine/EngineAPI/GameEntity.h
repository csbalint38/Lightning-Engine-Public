#pragma once

#include "../Components/ComponentsCommonHeaders.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"
#include "GeometryComponent.h"


namespace lightning::game_entity {

	DEFINE_TYPED_ID(entity_id);

	class Entity {
		private:
			entity_id _id;
		public:
			constexpr explicit Entity(entity_id id) : _id{ id } {}
			constexpr Entity() : _id{ id::invalid_id } {}
			[[nodiscard]] constexpr entity_id get_id() const { return _id; }
			[[nodiscard]] constexpr bool is_valid() const { return id::is_valid(_id); }
			[[nodiscard]] transform::Component transform() const;
			[[nodiscard]] script::Component script() const;
			[[nodiscard]] geometry::Component geometry() const;
			[[nodiscard]] math::v4 rotation() const { return transform().rotation(); }
			[[nodiscard]] math::v3 orientation() const { return transform().orientation(); }
			[[nodiscard]] math::v3 position() const { return transform().position(); }
			[[nodiscard]] math::v3 scale() const { return transform().scale(); }
		};
	}

namespace lightning::script {
	class EntityScript : public game_entity::Entity {
	public:
		virtual ~EntityScript() = default;
		virtual void begin_play() {}
		virtual void update(f32) {}
	protected:
		constexpr explicit EntityScript(game_entity::Entity entity) : game_entity::Entity{ entity.get_id() } {}

		void set_rotation(math::v4 rotation_quaternion) { set_rotation(this, rotation_quaternion); }
		void set_orientation(math::v3 orientation) { set_orientation(this, orientation); }
		void set_position(math::v3 position) { set_position(this, position); }
		void set_scale(math::v3 scale) { set_scale(this, scale); }

		static void set_rotation(const game_entity::Entity* const entity, math::v4 rotation_quaternion);
		static void set_orientation(const game_entity::Entity* const entity, math::v3 orientation_vector);
		static void set_position(const game_entity::Entity* const entity, math::v3 position);
		static void set_scale(const game_entity::Entity* const entity, math::v3 scale);
	};

	namespace detail {
		using script_ptr = std::unique_ptr<EntityScript>;
		using script_creator = script_ptr(*)(game_entity::Entity entity);
		using string_hash = std::hash<std::string>;

		u8 register_script(size_t, script_creator);
		#ifdef USE_WITH_EDITOR
		extern "C" __declspec(dllexport)
		#endif
		script_creator get_script_creator_from_engine(size_t tag);

		template<class ScriptClass>
		script_ptr create_script(game_entity::Entity entity) {
			assert(entity.is_valid());
			return std::make_unique<ScriptClass>(entity);
		}

		#ifdef USE_WITH_EDITOR
		u8 add_script_name(const char* name);

		#define REGISTER_SCRIPT(TYPE)																			\
		namespace {																								\
			const u8 _reg_##TYPE{																				\
				lightning::script::detail::register_script(lightning::script::detail::string_hash()(#TYPE), &lightning::script::detail::create_script<TYPE>)											  \
			};																									\
			const u8 _name_##TYPE																				\
			{ lightning::script::detail::add_script_name(#TYPE) };												\
		}
		#else
		#define REGISTER_SCRIPT(TYPE)																			\
		namespace {																								\
			const u8 _reg_##TYPE {																				\
				lightning::script::detail::register_script(lightning::script::detail::string_hash()(#TYPE), &lightning::script::detail::create_script<TYPE>)											  \
			};																									\
		}
		#endif
	}
}
