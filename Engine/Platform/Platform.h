#pragma once
#include "CommonHeaders.h"
#include "Window.h"

namespace lightning::platform {

	struct WindowInitInfo;

	Window create_window(const WindowInitInfo* const init_info = nullptr);
	void remove_window(window_id id);
}