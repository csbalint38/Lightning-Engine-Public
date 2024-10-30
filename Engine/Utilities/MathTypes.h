#pragma once
#include "CommonHeaders.h"

namespace lightning::math {
	constexpr f32 PI{ 3.14159274101257324219f };
	constexpr f32 INV_PI{ 1.f / PI };
	constexpr f32 TWO_PI{ 2.f * PI };
	constexpr f32 HALF_PI{ PI * .5f };
	constexpr f32 INV_TWO_PI{ 1.f / TWO_PI };
	constexpr f32 EPSILON{ 1e-5f };
	
	#if defined(_WIN64)
	using v2 = DirectX::XMFLOAT2;
	using v2a = DirectX::XMFLOAT2A;
	using v3 = DirectX::XMFLOAT3;
	using v3a = DirectX::XMFLOAT3A;
	using v4 = DirectX::XMFLOAT4;
	using v4a = DirectX::XMFLOAT4A;
	using u32v2 = DirectX::XMUINT2;
	using u32v3 = DirectX::XMUINT3;
	using u32v4 = DirectX::XMUINT4;
	using s32v2 = DirectX::XMINT2;
	using s32v3 = DirectX::XMINT3;
	using s32v4 = DirectX::XMINT4;
	using m3x3 = DirectX::XMFLOAT3X3;
	using m4x4 = DirectX::XMFLOAT4X4;
	using m4x4a = DirectX::XMFLOAT4X4A;
	#endif
}