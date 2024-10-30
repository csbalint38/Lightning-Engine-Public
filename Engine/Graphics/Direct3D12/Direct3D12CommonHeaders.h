#pragma once
#include "CommonHeaders.h"
#include "Graphics/Renderer.h"
#include "Platform/Window.h"

#ifndef  NOMINMAX
#define NOMINMAX
#endif


#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace lightning::graphics::direct3d12 {
	constexpr u32 FRAME_BUFFER_COUNT{ 3 }; // MIN: 2
	using id3d12_device = ID3D12Device10;
	using id3d12_graphics_command_list = ID3D12GraphicsCommandList7;
}

#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)								\
	if (FAILED(x)) {							\
		char line_number[32];					\
		sprintf_s(line_number, "%u", __LINE__);	\
		OutputDebugStringA("Error in: ");		\
		OutputDebugStringA(__FILE__);			\
		OutputDebugStringA("\nLine: ");			\
		OutputDebugStringA(line_number);		\
		OutputDebugStringA("\n");				\
		OutputDebugStringA(#x);					\
		OutputDebugStringA("\n");				\
		__debugbreak();							\
	}
#endif
#else
#ifndef DXCall
#define DXCall(x) x
#endif
#endif

#ifdef _DEBUG
#define NAME_D3D12_OBJECT(obj, name) {						\
	obj->SetName(name);										\
	OutputDebugStringW(L"::D3D12 Object Created: ");		\
	OutputDebugStringW(name);								\
	OutputDebugStringW(L"\n");								\
}
#define NAME_D3D12_OBJECT_INDEXED(obj, n, name) {				\
	wchar_t full_name[128];										\
	if (swprintf_s(full_name, L"%s[%llu]", name, (u64)n) > 0) {	\
		obj->SetName(full_name);								\
		OutputDebugStringW(L"::D3D12 Object Created: ");		\
		OutputDebugStringW(full_name);							\
		OutputDebugStringW(L"\n");								\
	}															\
}
	
#else
#define NAME_D3D12_OBJECT(x, name)
#define NAME_D3D12_OBJECT_INDEXED(x, n, name)
#endif

#include "Direct3D12Helpers.h"
#include "Direct3D12Resources.h"
