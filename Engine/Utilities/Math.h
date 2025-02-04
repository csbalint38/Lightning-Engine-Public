#pragma once

#include "CommonHeaders.h"

#if defined(_WIN64)
#include <DirectXMath.h>
#endif

#include "MathTypes.h"

namespace lightning::math {

	[[nodiscard]] constexpr bool is_equal(f32 a, f32 b, f32 eps = EPSILON) {
		f32 diff{ a - b };
		if (diff < 0.f) diff = -diff;
		return diff < eps;
	}

	template<typename T> [[nodiscard]] constexpr T clamp(T value, T min, T max) {
		assert(min <= max);
		return (value < min) ? min : (value > max) ? max : value;
	}
	
	template<u32 bits> [[nodiscard]] constexpr u32 pack_unit_float(f32 f) {
		static_assert(bits && bits <= sizeof(u32) * 8);
		assert(f >= 0.f && f <= 1.f);
		constexpr f32 intervals{ (f32)(((u32)1 << bits) - 1) };
		return (u32)(intervals * f + .5f);
	}
	
	template<u32 bits> [[nodiscard]] constexpr f32 unpack_to_unit_float(u32 i) {
		static_assert(bits && bits <= sizeof(u32) * 8);
		assert(i < ((u32)1) << bits);
		constexpr f32 intervals{ (f32)(((u32)1 << bits) - 1) };
		return (f32)i / intervals;
	}
	
	template<u32 bits> [[nodiscard]] constexpr u32 pack_float(f32 f, f32 min, f32 max) {
		assert(min < max);
		assert(f <= max && f >= min);
		const f32 distance{ (f - min) / (max - min) };
		return pack_unit_float<bits>(distance);
	}

	template<u32 bits> [[nodiscard]] constexpr f32 unpack_to_float(u32 i, f32 min, f32 max) {
		assert(min < max);
		return unpack_to_unit_float<bits>(i) * (max - min) + min;
	}

	template<u64 alignment> [[nodiscard]] constexpr u64 align_size_up(u64 size) {
		static_assert(alignment, "Alignment must be non-zero.");
		constexpr u64 mask{ alignment - 1 };
		static_assert(!(alignment & mask), "Alignment should be a power of 2.");
		return ((size + mask) & ~mask);
 	}

	template<u64 alignment> [[nodiscard]] constexpr u64 align_size_down(u64 size) {
		static_assert(alignment, "Alignment must be non-zero.");
		constexpr u64 mask{ alignment - 1 };
		static_assert(!(alignment & mask), "Alignment should be a power of 2.");
		return (size & ~mask);
	}

	[[nodiscard]] constexpr u64 align_size_up(u64 size, u64 alignment) {
		assert(alignment && "Alignment must be non-zero.");
		const u64 mask{ alignment - 1 };
		assert(!(alignment & mask) && "Alignment should be a power of 2.");
		return ((size + mask) & ~mask);
	}

	[[nodiscard]] constexpr u64 align_size_down(u64 size, u64 alignment) {
		assert(alignment && "Alignment must be non-zero.");
		const u64 mask{ alignment - 1 };
		assert(!(alignment & mask) && "Alignment should be a power of 2.");
		return (size & ~mask);
	}

	[[nodiscard]] constexpr u64 calc_crc32_u64(const u8* const data, u64 size) {
		assert(size >= sizeof(u64));

		u64 crc{ 0 };
		const u8* at{ data };
		const u8* const end{ data + align_size_down<sizeof(u64)>(size) };

		while (at < end) {
			crc = _mm_crc32_u64(crc, *((const u64*)at));
			at += sizeof(u64);
		}

		return crc;
 	}
}