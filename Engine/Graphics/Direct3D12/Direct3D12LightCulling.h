#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12 {
	struct D3D12FrameInfo;
}

namespace lightning::graphics::direct3d12::delight {
	constexpr u32 light_culling_tile_size{ 32 };

	bool initialize();
	void shutdown();
	[[nodiscard]] id::id_type add_culler();
	void remove_culler(id::id_type id);
	void cull_lights(id3d12_graphics_command_list* const cmd_list, const D3D12FrameInfo& info, d3dx::D3D12ResourceBarrier& barriers);

	// TEMP
	D3D12_GPU_VIRTUAL_ADDRESS frustums(id::id_type light_culling_id, u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS light_grid_opaque(id::id_type light_culling_id, u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS light_index_list_opaque(id::id_type light_culling_id, u32 frame_index);
}