#pragma once
#include "ToolsCommon.h"

namespace lightning::tools {

	struct Vertex {
		math::v4 tangent{};
		math::v4 joint_weights{};
		math::u32v4 joint_indicies{ u32_invalid_id, u32_invalid_id, u32_invalid_id, u32_invalid_id };
		math::v3 position{};
		math::v3 normal{};
		math::v2 uv{};;
		u8 red{};
		u8 green{};
		u8 blue{};
		u8 pad{};
	};

	namespace elements {
		struct ElementsType {
			enum Type : u32 {
				POSITION_ONLY = 0x00,
				STATIC_NORMAL = 0x01,
				STATIC_NORMAL_TEXTURE = 0x03,
				STATIC_COLOR = 0x04,
				SKELETAL = 0x08,
				SKELETAL_COLOR = SKELETAL | STATIC_COLOR,
				SKELETAL_NORMAL = SKELETAL | STATIC_NORMAL,
				SKELETAL_NORMAL_COLOR = SKELETAL_NORMAL | STATIC_COLOR,
				SKELETAL_NORMAL_TEXTURE = SKELETAL | STATIC_NORMAL_TEXTURE,
				SKELETAL_NORMAL_TEXTURE_COLOR = SKELETAL_NORMAL_TEXTURE | STATIC_COLOR
			};
		};

		struct StaticColor {
			u8 color[3];
			u8 pad;
		};

		struct StaticNormal {
			u8 color[3];
			u8 t_sign;
			u16 normal[2];
		};

		struct StaticNormalTexture {
			u8 color[3];
			u8 t_sign;
			u16 normal[2];
			u16 tangent[2];
			math::v2 uv;
		};

		struct Skeletal {
			u8 joint_weights[3];
			u8 pad;
			u16 joint_indicies[4];
		};

		struct SkeletalColor {
			u8 joint_weights[3];
			u8 pad;
			u16 joint_indicies[4];
			u8 color[3];
			u8 pad2;
		};

		struct SkeletalNormal {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indicies[4];
			u16 normal[2];
		};

		struct SkeletalNormalColor {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indicies[4];
			u16 normal[2];
			u8 color[3];
			u8 pad;
		};

		struct SkeletalNormalTexture {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indicies[4];
			u16 normal[2];
			u16 tangent[2];
			math::v2 uv;
		};

		struct SkeletalNormalTextureColor {
			u8 joint_weights[3];
			u8 t_sign;
			u16 joint_indicies[4];
			u16 normal[2];
			u16 tangent[2];
			math::v2 uv;
			u8 color[3];
			u8 pad;
		};
	}

	struct Mesh {
		util::vector<math::v3> positions;
		util::vector<math::v3> normals;
		util::vector<math::v4> tangents;
		util::vector<math::v3> colors;
		util::vector<util::vector<math::v2>> uv_sets;
		util::vector<u32> material_indicies;
		util::vector<u32> material_used;
		util::vector<u32> raw_indicies;

		util::vector<Vertex> verticies;
		util::vector<u32> indicies;

		std::string name;
		elements::ElementsType::Type elements_type;
		util::vector<u8> position_buffer;
		util::vector<u8> element_buffer;
		f32 lod_threshold{ -1.f };
		u32 lod_id{ u32_invalid_id };
	};

	struct LodGroup {
		std::string name;
		util::vector<Mesh> meshes;
	};

	struct Scene {
		std::string name;
		util::vector<LodGroup> lod_groups;
	};

	struct GeometryImportSettings {
		f32 smoothing_angle;
		u8 calculate_normals;
		u8 calculate_tangents;
		u8 reverse_handedness;
		u8 import_embeded_textures;
		u8 import_animations;
		u8 coalesce_meshes;
	};

	struct SceneData {
		u8* buffer;
		u32 buffer_size;
		GeometryImportSettings settings;
	};

	void process_scene(Scene& scene, const GeometryImportSettings& settings, Progression* const progression);
	void pack_data(const Scene& scene, SceneData& data);
	bool coalesce_meshes(const LodGroup& lod, Mesh& combined_mesh, Progression* const progression);
}