#pragma once

#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12 {
	struct D3D12FrameInfo;
}

namespace lightning::graphics::direct3d12::fx {
	bool initialize();
	void shutdown();

	void post_process(id3d12_graphics_command_list* cmd_list, const D3D12FrameInfo& info, D3D12_CPU_DESCRIPTOR_HANDLE target_rtv);
}