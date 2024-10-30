#if !defined(SHIPPING) && defined(_WIN64)

#include <thread>

#include "Content/ContentLoader.h"
#include "Components/Script.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Graphics/Renderer.h"

using namespace lightning;

namespace {

	graphics::RenderSurface game_window{};

	LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		switch (msg) {
		case WM_DESTROY: {
			if (game_window.window.is_closed()) {
				PostQuitMessage(0);
				return 0;
			}
		}
		case WM_SYSCHAR:
			if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
				game_window.window.set_fullscreen(!game_window.window.is_fullscreen());
				return 0;
			}
			break;
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
}

bool engine_initialize() {
	if (!content::load_game()) return false;
	
	platform::WindowInitInfo info{ &win_proc, nullptr, L"Lightning Game"};
	game_window.window = platform::create_window(&info);

	if (!game_window.window.is_valid()) return false;

	return true;
}

void engine_update() {
	script::update(10.f);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engine_shutdown() {
	platform::remove_window(game_window.window.get_id());
	content::unload_game();
}
#endif