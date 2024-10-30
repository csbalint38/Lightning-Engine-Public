#pragma once
namespace lightning::graphics {
	struct PlatformInterface;

	namespace direct3d12 {
		void get_platform_interface(PlatformInterface& pi);
	}
}