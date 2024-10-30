#include "Platform.h"
#include "PlatformTypes.h"
#include "Input/InputWin32.h"

namespace lightning::platform {
	#ifdef _WIN64

	namespace {

		struct WindowInfo {
			HWND hwnd{ nullptr };
			RECT client_area{ 0, 0, 1920, 1080 };
			RECT fullscreen_area{};
			POINT top_left{ 0, 0 };
			DWORD style{ WS_VISIBLE };
			bool is_fullscreen{ false };
			bool is_closed{ false };
		};

		util::free_list<WindowInfo> windows;
		

		WindowInfo& get_from_id(window_id id) {
			assert(windows[id].hwnd);
			return windows[id];
		}

		WindowInfo& get_from_handle(window_handle handle) {
			const window_id id{ (id::id_type)GetWindowLongPtrW(handle, GWLP_USERDATA) };
			return get_from_id(id);
		}

		bool resized{ false };
		LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

			switch (msg) {
				case WM_NCCREATE: {
					DEBUG_OP(SetLastError(0));
					const window_id id{ windows.add() };
					windows[id].hwnd = hwnd;
					SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)id);
					assert(GetLastError() == 0);
				}
				break;
				case WM_DESTROY:
					get_from_handle(hwnd).is_closed = true;
					break;
				case WM_SIZE:
					resized = (wparam != SIZE_MINIMIZED);
					break;
				default:
					break;
			}

			input::process_input_message(hwnd, msg, wparam, lparam);

			if (resized && GetKeyState(VK_LBUTTON) >= 0) {
				WindowInfo& info{ get_from_handle(hwnd) };
				assert(info.hwnd);
				GetClientRect(info.hwnd, info.is_fullscreen ? &info.fullscreen_area : &info.client_area);
				resized = false;
			}

			if (msg == WM_SYSCOMMAND && wparam == SC_KEYMENU) {
				return 0;
			}

			LONG_PTR long_ptr{ GetWindowLongPtrW(hwnd, 0) };
			return long_ptr ? ((window_proc)long_ptr)(hwnd, msg, wparam, lparam) : DefWindowProcW(hwnd, msg, wparam, lparam);
		}

		void resize_window(const WindowInfo& info, const RECT& area) {
			RECT window_rect{ area };
			AdjustWindowRect(&window_rect, info.style, FALSE);

			const s32 width{ window_rect.right - window_rect.left };
			const s32 height{ window_rect.bottom - window_rect.top };

			MoveWindow(info.hwnd, info.top_left.x, info.top_left.y, width, height, true);
		}

		void resize_window(window_id id, u32 width, u32 height) {
			WindowInfo& info{ get_from_id(id) };

			RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
			area.bottom = area.top + height;
			area.left = area.right + width;

			resize_window(info, area);
		}

		void set_window_fullscreen(window_id id, bool is_fullscreen) {
			WindowInfo& info{ get_from_id(id) };

			if (info.is_fullscreen != is_fullscreen) {
				info.is_fullscreen = is_fullscreen;
			}

			if (is_fullscreen) {
				GetClientRect(info.hwnd, &info.client_area);
				RECT rect;
				GetWindowRect(info.hwnd, &rect);
				info.top_left.x = rect.left;
				info.top_left.y = rect.top;
				SetWindowLongPtrW(info.hwnd, GWL_STYLE, 0);
				ShowWindow(info.hwnd, SW_MAXIMIZE);
			}
			else {
				SetWindowLongPtrW(info.hwnd, GWL_STYLE, info.style);
				resize_window(info, info.client_area);
				ShowWindow(info.hwnd, SW_SHOWNORMAL);
			}
		}
	}

	static bool is_window_fullscreen(window_id id) {
		return get_from_id(id).is_fullscreen;
	}

	static window_handle get_window_handle(window_id id) {
		return get_from_id(id).hwnd;
	}

	static void set_window_caption(window_id id, const wchar_t* caption) {
		WindowInfo& info{ get_from_id(id) };
		SetWindowTextW(info.hwnd, caption);
	}

	static math::u32v4 get_window_size(window_id id) {
		WindowInfo& info{ get_from_id(id) };
		RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };

		return { (u32)area.left, (u32)area.top, (u32)area.right, (u32)area.bottom };
	}

	static bool is_window_closed(window_id id) {
		return get_from_id(id).is_closed;
	}

	Window create_window(const WindowInitInfo* init_info) {
		window_proc callback{ init_info ? init_info->callback : nullptr };
		window_handle parent{ init_info ? init_info->parent : nullptr };

		WNDCLASSEXW wc;
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = internal_window_proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = callback ? sizeof(callback) : 0;
		wc.hInstance = 0;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"LightningWindow";
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		RegisterClassExW(&wc);

		WindowInfo info{};
		info.client_area.right = (init_info && init_info->width) ? info.client_area.left + init_info->width : info.client_area.right;
		info.client_area.bottom = (init_info && init_info->height) ? info.client_area.top + init_info->height : info.client_area.bottom;
		RECT rect{ info.client_area };
		info.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;
		
		AdjustWindowRect(&rect, info.style, FALSE);

		const wchar_t* caption{ (init_info && init_info->caption) ? init_info->caption : L"Lightning Game" };
		const s32 left{ init_info ? init_info->left : info.top_left.x};
		const s32 top{ init_info ? init_info->top : info.top_left.y };
		const s32 width{ rect.right - rect.left };
		const s32 height{ rect.bottom - rect.top };

		info.hwnd = CreateWindowExW(0, wc.lpszClassName, caption, info.style, left, top, width, height, parent, NULL, NULL, NULL);

		if (info.hwnd) {

			DEBUG_OP(SetLastError(0));
			if (callback) SetWindowLongPtrW(info.hwnd, 0, (LONG_PTR)callback);
			assert(GetLastError() == 0);

			ShowWindow(info.hwnd, SW_SHOWNORMAL);
			UpdateWindow(info.hwnd);

			window_id id{ (id::id_type)GetWindowLongPtr(info.hwnd, GWLP_USERDATA) };
			windows[id] = info;

			return Window{ id };
		}

		return {};
	}

	void remove_window(window_id id) {
		WindowInfo& info{ get_from_id(id) };
		DestroyWindow(info.hwnd);
		windows.remove(id);
	}
}
#include "IncludeWindowCpp.h"
#endif