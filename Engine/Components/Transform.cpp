#include "Entity.h"
#include "Transform.h"

namespace lightning::transform {
	namespace {
		util::vector<math::m4x4> to_world;
		util::vector<math::m4x4> inv_world;
		util::vector<math::v3> positions;
		util::vector<math::v3> orientations;
		util::vector<math::v4> rotations;
		util::vector<math::v3> scales;
		util::vector<u8> has_transform;
		util::vector<u8> changes_from_previous_frame;
		u8 read_write_flags;
	}

	void calculate_transform_matrices(id::id_type index) {
		assert(positions.size() >= index);
		assert(rotations.size() >= index);
		assert(scales.size() >= index);

		using namespace DirectX;
		XMVECTOR r{ XMLoadFloat4(&rotations[index]) };
		XMVECTOR p{ XMLoadFloat3(&positions[index]) };
		XMVECTOR s{ XMLoadFloat3(&scales[index]) };

		XMMATRIX world{ XMMatrixAffineTransformation(s, XMQuaternionIdentity(), r, p) };
		XMStoreFloat4x4(&to_world[index], world);

		world.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		XMMATRIX inverse_world{ XMMatrixInverse(nullptr, world) };
		XMStoreFloat4x4(&inv_world[index], inverse_world);

		has_transform[index] = 1;
	}

	math::v3 calculate_orientation(math::v4 rotation) {
		using namespace DirectX;
		XMVECTOR rotation_quat{ XMLoadFloat4(&rotation) };
		XMVECTOR front{ XMVectorSet(0.f, 0.f, 1.f, 0.f) };
		math::v3 orientation;
		XMStoreFloat3(&orientation, XMVector3Normalize(XMVector3Rotate(front, rotation_quat)));
		return orientation;
	}

	void set_rotation(transform_id id, const math::v4& rotaion_quaternion) {
		const u32 index{ id::index(id) };
		rotations[index] = rotaion_quaternion;
		orientations[index] = calculate_orientation(rotaion_quaternion);
		has_transform[index] = 0;
		changes_from_previous_frame[index] |= ComponentFlags::ROTATION;
	}

	void set_orientation(transform_id, const math::v3&) {}

	void set_position(transform_id id, const math::v3& position) {
		const u32 index{ id::index(id) };
		positions[index] = position;
		has_transform[index] = 0;
		changes_from_previous_frame[index] |= ComponentFlags::POSITION;
	}

	void set_scale(transform_id id, const math::v3& scale) {
		const u32 index{ id::index(id) };
		scales[index] = scale;
		has_transform[index] = 0;
		changes_from_previous_frame[index] |= ComponentFlags::SCALE;
	}

	Component create(InitInfo info, game_entity::Entity entity) {
		assert(entity.is_valid());
		const id::id_type entity_index{ id::index(entity.get_id()) };

		if (positions.size() > entity_index) {
			math::v4 rotation{ info.rotation };
			rotations[entity_index] = rotation;
			orientations[entity_index] = calculate_orientation(rotation);
			positions[entity_index] = math::v3{ info.position };
			scales[entity_index] = math::v3{ info.scale };
			has_transform[entity_index] = 0;
			changes_from_previous_frame[entity_index] = (u8)ComponentFlags::ALL;
		}
		else {
			assert(positions.size() == entity_index);
			rotations.emplace_back(info.rotation);
			orientations.emplace_back(calculate_orientation(math::v4{ info.rotation }));
			positions.emplace_back(info.position);
			scales.emplace_back(info.scale);
			has_transform.emplace_back((u8)0);
			to_world.emplace_back();
			inv_world.emplace_back();
			changes_from_previous_frame.emplace_back((u8)ComponentFlags::ALL);
		}
		return Component{ transform_id{ entity.get_id()} };
	}

	void remove([[maybe_unused]] Component component) {
		assert(component.is_valid());
	}

	void get_transform_matrices(const game_entity::entity_id id, math::m4x4& world, math::m4x4& inverse_world) {
		assert(game_entity::Entity{ id }.is_valid());

		const id::id_type entity_index{ id::index(id) };
		if (!has_transform[entity_index]) {
			calculate_transform_matrices(entity_index);
		}

		world = to_world[entity_index];
		inverse_world = inv_world[entity_index];
	}

	void get_updated_component_flags(const game_entity::entity_id* const ids, u32 count, u8* const flags) {
		assert(ids && count && flags);
		read_write_flags = 1;

		for (u32 i{ 0 }; i < count; ++i) {
			assert(game_entity::Entity{ ids[i] }.is_valid());
			flags[i] = changes_from_previous_frame[id::index(ids[i])];
		}
	}

	void update(const ComponentCache* const cache, u32 count) {
		assert(cache && count);
		if (read_write_flags) {
			memset(changes_from_previous_frame.data(), 0, changes_from_previous_frame.size());
			read_write_flags = 0;
		}

		for (u32 i{ 0 }; i < count; ++i) {
			const ComponentCache& c{ cache[i] };
			assert(Component{ c.id }.is_valid());

			if (c.flags & ComponentFlags::ROTATION) {
				set_rotation(c.id, c.rotation);
			}

			if (c.flags & ComponentFlags::ORIENTATION) {
				set_orientation(c.id, c.orientation);
			}

			if (c.flags & ComponentFlags::POSITION) {
				set_position(c.id, c.position);
			}

			if (c.flags & ComponentFlags::SCALE) {
				set_scale(c.id, c.scale);
			}
		}
	}

	math::v3 Component::position() const {
		assert(is_valid());
		return positions[id::index(_id)];
	}
	
	math::v4 Component::rotation() const {
		assert(is_valid());
		return rotations[id::index(_id)];
	}

	math::v3 Component::orientation() const {
		assert(is_valid());
		return orientations[id::index(_id)];
	}

	math::v3 Component::scale() const {
		assert(is_valid());
		return scales[id::index(_id)];
	}
}