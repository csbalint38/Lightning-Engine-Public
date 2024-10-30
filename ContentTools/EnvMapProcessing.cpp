#include "ToolsCommon.h"
#include <DirectXTex.h>
#include <dxgi1_6.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace lightning::tools {
	namespace {
		namespace shaders {
			#include "EnvMapProcessing_equirectangular_to_cube_map_cs.inc"
		};

		constexpr u32 prefiltered_diffuse_cubemap_size{ 64 };
		constexpr u32 prefiltered_specular_cubemap_size{ 256 };
		constexpr u32 roughness_mip_levels{ 6 };
		constexpr u32 brdf_integration_lut_size{ 256 };

		struct ShaderConstants {
			u32 cube_map_in_size;
			u32 cube_map_out_size;
			u32 sample_count;
			f32 roughness;
			
			private:
		};

		math::v3 get_sample_direction_equirectangular(u32 face, f32 u, f32 v) {
			math::v3 directions[6]{
				{ -u, 1.f, -v },	// x+ left
				{ u, -1.f, -v },	// x- right
				{ v, u, 1.f },		// y+ bottom
				{ -v, u, -1.f },	// y- top
				{ 1.f, u, -v },		// z+ front
				{ -1.f, -u, -v },	// z- back
			};

			XMVECTOR dir{ XMLoadFloat3(&directions[face]) };
			dir = XMVector3Normalize(dir);
			math::v3 normalized_dir;
			XMStoreFloat3(&normalized_dir, dir);

			return normalized_dir;
		}

		math::v2 direction_to_equirectangular(const math::v3& dir) {
			const f32 phi{ atan2f(dir.y, dir.x) };
			const f32 theta{ XMScalarACos(dir.z) };
			const f32 s{ phi * math::INV_TWO_PI + .5f };
			const f32 t{ theta * math::INV_PI };

			return { s, t };
		}

		void sample_cube_face(const Image& env_map, const Image& cube_face, u32 face_index, bool mirror) {
			assert(cube_face.width == cube_face.height);

			const f32 inv_width{ 1.f / (f32)cube_face.height };
			const f32 inv_height{ 1.f / (f32)cube_face.width };
			const u32 row_pitch{ (u32)cube_face.rowPitch };
			const u32 env_width{ (u32)env_map.width -1 };
			const u32 env_height{ (u32)env_map.height - 1 };
			const u32 env_row_pitch{ (u32)env_map.rowPitch };

			constexpr u32 bytes_per_pixel{ sizeof(f32) * 4 };

			for (u32 y{ 0 }; y < cube_face.height; ++y) {
				const f32 v{ (y * inv_height) * 2.f - 1.f };

				for (u32 x{ 0 }; x < cube_face.width; ++x) {
					const f32 u{ (x * inv_width) * 2.f - 1.f };
					const math::v3 sample_direction{ get_sample_direction_equirectangular(face_index, u, v) };
					math::v2 uv{ direction_to_equirectangular(sample_direction) };

					assert(uv.x >= 0.f && uv.x <= 1.f && uv.y >= 0.f && uv.y <= 1.f);

					if (mirror) uv.x = 1.f - uv.x;
					const f32 pos_x{ uv.x * env_width };
					const f32 pos_y{ uv.y * env_height };
					u8* dst_pixel{ &cube_face.pixels[row_pitch * y + x * bytes_per_pixel] };
					u8* const src_pixel{ &env_map.pixels[env_row_pitch * (u32)pos_y + (u32)pos_x * bytes_per_pixel] };
					memcpy(dst_pixel, src_pixel, bytes_per_pixel);
				}
			}
		}

		void reset_d3d11_context(ID3D11DeviceContext* ctx) {
			u8 zero_mem_block[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT * sizeof(void*)];
			memset(&zero_mem_block, 0, sizeof(zero_mem_block));

			ctx->CSSetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, (ID3D11UnorderedAccessView**)&zero_mem_block[0], nullptr);
			ctx->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, (ID3D11ShaderResourceView**)&zero_mem_block[0]);
			ctx->CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)&zero_mem_block[0]);
		}

		HRESULT set_constants(ID3D11DeviceContext* ctx, ID3D11Buffer* constant_buffer, ShaderConstants constants) {
			D3D11_MAPPED_SUBRESOURCE mapped_buffer{};
			HRESULT hr{ ctx->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_buffer) };

			if (FAILED(hr) || !mapped_buffer.pData) {
				return hr;
			}

			memcpy(mapped_buffer.pData, &constants, sizeof(ShaderConstants));
			ctx->Unmap(constant_buffer, 0);
			
			return hr;
		}

		HRESULT create_constant_buffer(ID3D11Device* device, ID3D11Buffer** constant_buffer) {
			D3D11_BUFFER_DESC desc{};
			desc.ByteWidth = sizeof(ShaderConstants);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			HRESULT hr{ device->CreateBuffer(&desc, nullptr, constant_buffer) };
			return hr;
		}

		HRESULT create_linear_sampler(ID3D11Device* device, ID3D11SamplerState** linear_sampler) {
			D3D11_SAMPLER_DESC desc{};
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			return device->CreateSamplerState(&desc, linear_sampler);
		}

		HRESULT create_texture_2d_uav(ID3D11Device* device, DXGI_FORMAT format, u32 array_size, u32 first_array_slice, u32 mip_slice, ID3D11Resource* texture, ID3D11UnorderedAccessView** texture_uav) {
			D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.Format = format;
			desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.ArraySize = array_size;
			desc.Texture2DArray.FirstArraySlice = first_array_slice;
			desc.Texture2DArray.MipSlice = mip_slice;

			return device->CreateUnorderedAccessView(texture, &desc, texture_uav);
		}

		HRESULT download_texture_2d(ID3D11DeviceContext* ctx, u32 width, u32 height, u32 array_size, u32 mip_levels, DXGI_FORMAT format, bool is_cubemap, ID3D11Texture2D* gpu_resource, ID3D11Texture2D* cpu_resource, ScratchImage& result) {
			ctx->CopyResource(cpu_resource, gpu_resource);

			HRESULT hr{ is_cubemap ? result.InitializeCube(format, width, height, array_size / 6, mip_levels) : result.Initialize2D(format, width, height, array_size, mip_levels) };

			if (FAILED(hr)) {
				return hr;
			}

			for (u32 img_idx{ 0 }; img_idx < mip_levels; ++img_idx) {
				for (u32 mip{ 0 }; mip < mip_levels; ++mip) {
					D3D11_MAPPED_SUBRESOURCE mapped_resource{};
					const u32 resource_idx{ mip + (img_idx * mip_levels) };
					hr = ctx->Map(cpu_resource, resource_idx, D3D11_MAP_READ, 0, &mapped_resource);

					if (FAILED(hr)) {
						result.Release();
						return hr;
					}

					const Image& img{ *result.GetImage(mip, img_idx, 0) };
					u8* src{ (u8*)mapped_resource.pData };
					u8* dst{ img.pixels };

					for (u32 row{ 0 }; row < img.height; ++row) {
						memcpy(dst, src, img.rowPitch);
						src += mapped_resource.RowPitch;
						dst += img.rowPitch;
					}

					ctx->Unmap(cpu_resource, resource_idx);
				}
			}

			return hr;
		}

		void dispatch(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView** const srv_array, ID3D11UnorderedAccessView** const uav_array, ID3D11Buffer** const buffers_array, ID3D11SamplerState** const samplers_array, ID3D11ComputeShader* shader, math::u32v3 group_count) {
			ctx->CSSetShaderResources(0, 1, srv_array);
			ctx->CSSetUnorderedAccessViews(0, 1, uav_array, nullptr);
			ctx->CSSetConstantBuffers(0, 1, &buffers_array[0]);
			ctx->CSSetSamplers(0, 1, &samplers_array[0]);
			ctx->CSSetShader(shader, nullptr, 0);
			ctx->Dispatch(group_count.x, group_count.y, group_count.z);
		}
 	}

	HRESULT equirectangular_to_cubemap(const Image* env_maps, u32 env_map_count, u32 cubemap_size, bool use_prefilter_size, bool mirror_cubemap, ScratchImage& cube_maps) {
		if (use_prefilter_size) {
			cubemap_size = prefiltered_specular_cubemap_size;
		}

		assert(env_maps && env_map_count);

		HRESULT hr{ S_OK };

		ScratchImage working_scratch{};
		hr = working_scratch.InitializeCube(DXGI_FORMAT_R32G32B32A32_FLOAT, cubemap_size, cubemap_size, env_map_count, 1);

		if (FAILED(hr)) {
			return hr;
		}

		for (u32 i{ 0 }; i < env_map_count; ++i) {
			const Image& env_map{ env_maps[i] };

			assert(math::is_equal((f32)env_map.width / (f32)env_map.height, 2.f));

			ScratchImage f32_env_map{};

			if (env_maps[0].format != DXGI_FORMAT_R32G32B32A32_FLOAT) {
				hr = Convert(env_map, DXGI_FORMAT_R32G32B32A32_FLOAT, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, f32_env_map);

				if (FAILED(hr)) {
					return hr;
				}
			}
			else {
				f32_env_map.InitializeFromImage(env_map);
			}

			assert(f32_env_map.GetImageCount() == 1);

			const Image* dst_images{ &working_scratch.GetImages()[i * 6] };
			const Image& env_map_image{ f32_env_map.GetImages()[i] };
			const bool mirror{ mirror_cubemap };

			std::thread threads[]{
				std::thread { [&] { sample_cube_face(env_map_image, dst_images[0], 0, mirror); } },
				std::thread { [&] { sample_cube_face(env_map_image, dst_images[1], 1, mirror); } },
				std::thread { [&] { sample_cube_face(env_map_image, dst_images[2], 2, mirror); } },
				std::thread { [&] { sample_cube_face(env_map_image, dst_images[3], 3, mirror); } },
				std::thread { [&] { sample_cube_face(env_map_image, dst_images[4], 4, mirror); } },
			};

			sample_cube_face(f32_env_map.GetImages()[i], dst_images[5], 5, mirror);

			for (u32 face{ 0 }; face < 5; ++face) threads[face].join();
		}

		if (env_maps[0].format != DXGI_FORMAT_R32G32B32A32_FLOAT) {
			hr = Convert(working_scratch.GetImages(), working_scratch.GetImageCount(), working_scratch.GetMetadata(), env_maps[0].format, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, cube_maps);
		}
		else {
			cube_maps = std::move(working_scratch);
		}

		return hr;
	}

	HRESULT equirectangular_to_cubemap(ID3D11Device* device, const Image* env_maps, u32 env_map_count, u32 cubemap_size, bool use_prefilter_size, bool mirror_cubemap, ScratchImage& cubemaps_out) {
		if (use_prefilter_size) {
			cubemap_size = prefiltered_specular_cubemap_size;
		}

		assert(env_maps && env_map_count);
		const DXGI_FORMAT format{ env_maps[0].format };
		const u32 array_size{ env_map_count * 6 };
		ComPtr<ID3D11DeviceContext> ctx{};
		device->GetImmediateContext(ctx.GetAddressOf());
		assert(ctx.Get());

		HRESULT hr{ S_OK };

		ComPtr<ID3D11Texture2D> cubemaps{};
		ComPtr<ID3D11Texture2D> cubemaps_cpu{};

		{
			D3D11_TEXTURE2D_DESC desc{};
			desc.Width = desc.Height = cubemap_size;
			desc.MipLevels = 1;
			desc.ArraySize = array_size;
			desc.Format = format;
			desc.SampleDesc = { 1,0 };
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			hr = device->CreateTexture2D(&desc, nullptr, cubemaps.GetAddressOf());

			if (FAILED(hr)) {
				return hr;
			}

			desc.BindFlags = 0;
			desc.Usage = D3D11_USAGE_STAGING;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

			hr = device->CreateTexture2D(&desc, nullptr, cubemaps_cpu.GetAddressOf());

			if (FAILED(hr)) {
				return hr;
			}
		}

		ComPtr<ID3D11ComputeShader> shader{};
		hr = device->CreateComputeShader(shaders::EnvMapProcessing_equirectangular_to_cube_map_cs, sizeof(shaders::EnvMapProcessing_equirectangular_to_cube_map_cs), nullptr, shader.GetAddressOf());

		if (FAILED(hr)) {
			return hr;
		}

		ComPtr<ID3D11Buffer> constant_buffer{};
		{
			hr = create_constant_buffer(device, constant_buffer.GetAddressOf());

			if (FAILED(hr)) {
				return hr;
			}

			ShaderConstants constants{};
			constants.cube_map_out_size = cubemap_size;
			constants.sample_count = mirror_cubemap ? 1 : 0;

			hr = set_constants(ctx.Get(), constant_buffer.Get(), constants);

			if (FAILED(hr)) {
				return hr;
			}
		}

		ComPtr<ID3D11SamplerState> linear_sampler{};
		hr = create_linear_sampler(device, linear_sampler.GetAddressOf());

		if (FAILED(hr)) {
			return hr;
		}

		reset_d3d11_context(ctx.Get());

		for (u32 i{ 0 }; i < env_map_count; ++i) {
			ComPtr<ID3D11UnorderedAccessView> cubemap_uav{};
			hr = create_texture_2d_uav(device, env_maps[0].format, 6, i * 6, 0, cubemaps.Get(), cubemap_uav.ReleaseAndGetAddressOf());

			if (FAILED(hr)) {
				return hr;
			}

			const Image& src{ env_maps[i] };

			ComPtr<ID3D11Texture2D> env_map{};

			{
				D3D11_TEXTURE2D_DESC desc{};
				desc.Width = (u32)src.width;
				desc.Height = (u32)src.height;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = format;
				desc.SampleDesc = { 1, 0 };
				desc.Usage = D3D11_USAGE_IMMUTABLE;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;

				D3D11_SUBRESOURCE_DATA data{};
				data.pSysMem = src.pixels;
				data.SysMemPitch = (u32)src.rowPitch;

				hr = device->CreateTexture2D(&desc, &data, env_map.ReleaseAndGetAddressOf());

				if (FAILED(hr)) {
					return hr;
				}
			};

			ComPtr<ID3D11ShaderResourceView> env_map_srv{};
			hr = device->CreateShaderResourceView(env_map.Get(), nullptr, env_map_srv.ReleaseAndGetAddressOf());

			if (FAILED(hr)) {
				return hr;
			}

			const u32 block_size{ (cubemap_size + 15) >> 4 };
			dispatch(ctx.Get(), env_map_srv.GetAddressOf(), cubemap_uav.GetAddressOf(), constant_buffer.GetAddressOf(), linear_sampler.GetAddressOf(), shader.Get(), { block_size, block_size, 6 });
		}

		reset_d3d11_context(ctx.Get());

		return download_texture_2d(ctx.Get(), cubemap_size, cubemap_size, array_size, 1, format, true, cubemaps.Get(), cubemaps_cpu.Get(), cubemaps_out);
	}
}