#include "Direct3D12Shaders.h"
#include "Content/ContentLoader.h"
#include "Content/ContentToEngine.h"

namespace lightning::graphics::direct3d12::shaders {
	namespace {

		content::compiled_shader_ptr engine_shaders[EngineShader::count]{};
		std::unique_ptr<u8[]> engine_shaders_blob{};

		bool load_engine_shaders() {
			assert(!engine_shaders_blob);
			u64 size{ 0 };
			bool result{ content::load_engine_shaders(engine_shaders_blob, size) };
			assert(engine_shaders_blob && size);

			u64 offset{ 0 };
			u32 index{ 0 };

			while (offset < size && result) {
				assert(index < EngineShader::count);
				content::compiled_shader_ptr& shader{ engine_shaders[index] };
				assert(!shader);

				result &= index < EngineShader::count && !shader;

				if (!result) break;

				shader = reinterpret_cast<const content::compiled_shader_ptr>(&engine_shaders_blob[offset]);
				offset += shader->buffer_size();
				++index;
			}
			assert(offset == size && index == EngineShader::count);

			return result;
		}
	}

	bool initialize() {
		return load_engine_shaders();
	}

	void shutdown() {
		for (u32 i{ 0 }; i < EngineShader::count; ++i) {
			engine_shaders[i] = {};
		}
		engine_shaders_blob.reset();
	}

	D3D12_SHADER_BYTECODE get_engine_shader(EngineShader::Id id) {
		assert(id < EngineShader::count);
		const content::compiled_shader_ptr shader{ engine_shaders[id] };
		assert(shader && shader->byte_code_size());

		return { shader->byte_code(), shader->byte_code_size() };
	}
}