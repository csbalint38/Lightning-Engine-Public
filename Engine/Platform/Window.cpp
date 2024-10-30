#if INCLUDE_WINDOW_CPP

#include "Window.h"

namespace lightning::platform {
	void Window::set_fullscreen(bool is_fullscreen) const {
		assert(is_valid());
		set_window_fullscreen(_id, is_fullscreen);
	}

	bool Window::is_fullscreen() const {
		assert(is_valid());
		return is_window_fullscreen(_id);
	}

	void* Window::handle() const {
		assert(is_valid());
		return get_window_handle(_id);
	}

	void Window::set_caption(const wchar_t* caption) const {
		assert(is_valid());
		set_window_caption(_id, caption);
	}

	math::u32v4 Window::size() const {
		assert(is_valid());
		return get_window_size(_id);
	}

	void Window::resize(u32 width, u32 height) const {
		assert(is_valid());
		resize_window(_id, width, height);
	}

	u32 Window::width() const {
		math::u32v4 s{ size() };
		return s.z - s.x;
	}

	u32 Window::height() const {
		math::u32v4 s{ size() };
		return s.w - s.y;
	}

	bool Window::is_closed() const {
		assert(is_valid());
		return is_window_closed(_id);
	}
}
#endif