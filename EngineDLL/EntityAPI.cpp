#include "CommonHeaders.h"
#include "Id.h"
#include "Common.h"
#include "Components/Entity.h"
#include "Components/Transform.h"


using namespace lightning;

namespace {

	struct TransformComponentDescriptor {
		f32 position[3];
		f32 rotation[3];
		f32 scale[3];

		transform::InitInfo to_InitInfo() {
			using namespace DirectX;
			transform::InitInfo info{};
			memcpy(&info.position[0], &position[0], sizeof(position));
			memcpy(&info.scale[0], &position[0], sizeof(position));
			XMFLOAT3A rot{ &rotation[0] };
			XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
			XMFLOAT4A rot_quat{};
			XMStoreFloat4A(&rot_quat, quat);
			memcpy(&info.rotation[0], &rot_quat.x, sizeof(info.rotation));
			return info;
		}
	};

	struct EntityDescriptor {
		TransformComponentDescriptor transform;
	};

	game_entity::Entity entity_from_id(id::id_type id) {
		return game_entity::Entity{ game_entity::entity_id{id} };
	}
}

EDITOR_INTERFACE id::id_type create_game_entity(EntityDescriptor* entity) {
	assert(entity);
	EntityDescriptor& desc{ *entity };
	transform::InitInfo transform_info{ desc.transform.to_InitInfo() };
	game_entity::EntityInfo entity_info{ &transform_info };

	return game_entity::create(entity_info).get_id();
}

EDITOR_INTERFACE void remove_game_entity(id::id_type id) {
	assert(id::is_valid(id));
	game_entity::remove(game_entity::entity_id{ id });
}