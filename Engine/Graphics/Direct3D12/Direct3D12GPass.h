#pragma once

#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12 {
	struct D3D12FrameInfo;
}

namespace lightning::graphics::direct3d12::gpass {

	constexpr DXGI_FORMAT main_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
	constexpr DXGI_FORMAT depth_buffer_format{ DXGI_FORMAT_D32_FLOAT };

	struct OpaqueRootParameter {
		enum parameter : u32 {
			GLOBAL_SHADER_DATA,
			PER_OBJECT_DATA,
			POSITION_BUFFER,
			ELEMENT_BUFFER,
			SRV_INDICIES,
			DIRECTIONAL_LIGHTS,
			CULLABLE_LIGHTS,
			LIGHT_GRID,
			LIGHT_INDEX_LIST,

			count
		};
	};

	bool initialize();
	void shutdown();

	[[nodiscard]] const D3D12RenderTexture& main_buffer();
	[[nodiscard]] const D3D12DepthBuffer& depth_buffer();

	void set_size(math::u32v2 size);
	void depth_prepass(id3d12_graphics_command_list* cmd_list, const D3D12FrameInfo& info);
	void render(id3d12_graphics_command_list* cmd_list, const D3D12FrameInfo& info);

	void add_transitions_for_depth_prepass(d3dx::D3D12ResourceBarrier& barriers);
	void add_transitions_for_gpass(d3dx::D3D12ResourceBarrier& barriers);
	void add_transitions_for_post_process(d3dx::D3D12ResourceBarrier& barriers);

	void set_render_targets_for_depth_prepass(id3d12_graphics_command_list* cmd_list);
	void set_render_targets_for_gpass(id3d12_graphics_command_list* cmd_list);
}