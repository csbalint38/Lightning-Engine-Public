#pragma once
#include "CommonHeaders.h"

namespace lightning::content {

	struct AssetType {
		enum Type : u32 {
			UNKNOWN = 0,
			ANIMATION,
			AUDIO,
			MATERIAL,
			MESH,
			SKELETON,
			TEXTURE,

			count
		};
	};

	struct TextureFlags {
		enum Flags : u32 {
			IS_HDR = 0x01,
			HAS_ALPHA = 0x02,
			IS_PREMULTIPLIED_ALPHA = 0x04,
			IS_IMPORTED_AS_NORMAL_MAP = 0x08,
			IS_CUBE_MAP = 0x10,
			IS_VOLUME_MAP = 0x20,
			IS_SRGB = 0x40,
		};
	};

	typedef struct CompiledShader {
		static constexpr u32 hash_length{ 16 };
		constexpr u64 byte_code_size() const { return _byte_code_size; }
		constexpr const u8* const hash() const { return &_hash[0]; }
		constexpr const u8* const byte_code() const { return &_byte_code; }
		constexpr const u64 buffer_size() const { return sizeof(_byte_code_size) + hash_length + _byte_code_size; }
		constexpr static u64 buffer_size(u64 size) { return sizeof(_byte_code_size) + hash_length + size; }

		private:
			u64 _byte_code_size;
			u8 _hash[hash_length];
			u8 _byte_code;
	} const* compiled_shader_ptr;

	struct LodOffset {
		u16 offset;
		u16 count;
	};

	id::id_type create_resource(const void* const data, AssetType::Type type);
	void destroy_resource(id::id_type id, AssetType::Type type);

	id::id_type add_shader_group(const u8* const* shaders, u64 num_shaders, const u32* const keys);
	void remove_shader_group(id::id_type id);
	compiled_shader_ptr get_shader(id::id_type id, u32 shader_key);

	void get_submesh_gpu_ids(id::id_type geometry_content_id, u32 id_count, id::id_type* const gpu_ids);
	void get_lod_offsets(const id::id_type* const geometry_ids, const f32* const thresholds, u32 id_count, util::vector<LodOffset>& offsets);
}
