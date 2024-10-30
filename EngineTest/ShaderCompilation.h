#pragma once

#include "CommonHeaders.h"

struct ShaderType {
	enum Type : u32 {
		VERTEX = 0,
		HULL,
		DOMAIN,
		GEOMETRY,
		PIXEL,
		COMPUTE,
		AMPLIFICATION,
		MESH,

		count
	};
};


struct ShaderFileInfo {
	const char* file_name;
	const char* function;
	ShaderType::Type type;
};

std::unique_ptr<u8[]> compile_shader(ShaderFileInfo info, const char* file_path, lightning::util::vector<std::wstring>& extra_args);
bool compile_shaders();