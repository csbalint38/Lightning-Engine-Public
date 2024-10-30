#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12 {
	struct D3D12FrameInfo;
}

namespace lightning::graphics::direct3d12::light {
	bool initialize();
	void shutdown();

	void create_light_set(u64 light_set_key);
	void remove_light_set(u64 light_set_key);
	graphics::Light create(LightInitInfo info);
	void remove(light_id id, u64 light_set_key);
	void set_parameter(light_id id, u64 light_set_key, LightParameter::Parameter parameter, const void* const data, u32 data_size);
	void get_parameter(light_id id, u64 light_set_key, LightParameter::Parameter parameter, void* const data, u32 data_size);

	void update_light_buffers(const D3D12FrameInfo& info);
	D3D12_GPU_VIRTUAL_ADDRESS non_cullable_light_buffer(u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS cullable_light_buffer(u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS culling_info_buffer(u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS bounding_spheres_buffer(u32 frame_index);
	u32 non_cullable_light_count(u64 light_set_key);
	u32 cullable_light_count(u64 light_set_key);
}