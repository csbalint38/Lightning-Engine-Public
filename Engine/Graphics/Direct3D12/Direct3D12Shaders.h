#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12::shaders {

	struct EngineShader {
		enum Id : u32 {
			FULLSCREEN_TRIANGLE_VS = 0,
			FILL_COLOR_PS = 1,
			POST_PROCESS_PS = 2,
			GRID_FRUSTUMS_CS = 3,
			LIGHT_CULLING_CS = 4,

			count
		};
	};

	bool initialize();
	void shutdown();

	D3D12_SHADER_BYTECODE get_engine_shader(EngineShader::Id id);
}