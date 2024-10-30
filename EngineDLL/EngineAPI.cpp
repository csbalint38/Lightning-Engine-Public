#include "Common.h"
#include "CommonHeaders.h"
#include "Components/Script.h"
#include "Graphics/Renderer.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <atlsafe.h>

using namespace lightning;

namespace {
	HMODULE game_code_dll{ nullptr };
	using _get_script_creator = lightning::script::detail::script_creator(*)(size_t);
	_get_script_creator script_creator{ nullptr };
	using _get_script_names = LPSAFEARRAY(*)(void);
	_get_script_names script_names{ nullptr };

	util::vector<graphics::RenderSurface> surfaces;
}

EDITOR_INTERFACE u32 load_game_code_dll(const char* dll_path) {
	if (game_code_dll) return FALSE;
	game_code_dll = LoadLibraryA(dll_path);
	assert(game_code_dll);

	script_creator = (_get_script_creator)GetProcAddress(game_code_dll, "get_script_creator");
	script_names = (_get_script_names)GetProcAddress(game_code_dll, "get_script_names");

	return (game_code_dll && script_creator && script_names) ? TRUE : FALSE;
}
EDITOR_INTERFACE u32 unload_game_code_dll() {
	if (!game_code_dll) return FALSE;
	assert(game_code_dll);
	[[maybe_unused]] int result{ FreeLibrary(game_code_dll) };
	assert(result);
	game_code_dll = nullptr;
	return TRUE;
}

EDITOR_INTERFACE script::detail::script_creator get_script_creator(const char* name) {
	return (game_code_dll && script_creator) ? script_creator(script::detail::string_hash()(name)) : nullptr;
}

EDITOR_INTERFACE LPSAFEARRAY get_script_names() {
	return (game_code_dll && script_names) ? script_names() : nullptr;
}

EDITOR_INTERFACE u32 create_renderer_surface(HWND host, s32 width, s32 height) {
	assert(host);
	platform::WindowInitInfo info{ nullptr, host, nullptr, 0, 0, width, height };
	graphics::RenderSurface surface{ platform::create_window(&info), {} };
	assert(surface.window.is_valid());
	surfaces.emplace_back(surface);
	return (u32)surfaces.size() - 1;
}

EDITOR_INTERFACE void remove_renderer_surface(u32 id) {
	assert(id < surfaces.size());
	platform::remove_window(surfaces[id].window.get_id());
}

EDITOR_INTERFACE HWND get_window_handle(u32 id) {
	assert(id < surfaces.size());
	return (HWND)surfaces[id].window.handle();
}