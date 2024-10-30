#pragma once

#include "Test.h"
#include "..\Engine\Platform\Platform.h"
#include "..\Engine\Platform\PlatformTypes.h"

using namespace lightning;

platform::Window _windows[4];

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_DESTROY: {
			bool all_closed{ true };
			for (u32 i{ 0 }; i < _countof(_windows); ++i) {
				if (!_windows[i].is_closed()) {
					all_closed = false;
				}
			}
			if (all_closed) {
				PostQuitMessage(0);
				return 0;
			}
			break;
		}
		case WM_SYSCHAR:
			if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN)) {
				platform::Window win{ platform::window_id{(id::id_type)GetWindowLongPtrW(hwnd, GWLP_USERDATA)} };
				win.set_fullscreen(!win.is_fullscreen());
				return 0;
			}
			break;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

class EngineTest : Test {
	public:
		bool initialize() override {

			platform::WindowInitInfo info[]{
				{&win_proc, nullptr, L"TestWindow1", 100, 100, 800, 800},
				{&win_proc, nullptr, L"TestWindow2", 150, 150, 400, 800},
				{&win_proc, nullptr, L"TestWindow3", 200, 200, 800, 400},
				{&win_proc, nullptr, L"TestWindow4", 250, 250, 400, 400}
			};
			static_assert(_countof(_windows) == _countof(info));

			for (u32 i{ 0 }; i < _countof(_windows); ++i) {
				_windows[i] = platform::create_window(&info[i]);
			}
			return true;
		}

		void run() override {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		void shutdown() override {
			for (u32 i{ 0 }; i < _countof(_windows); ++i) {
				platform::remove_window(_windows[i].get_id());
			}
		}
};