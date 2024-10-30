#include "ToolsCommon.h"
#include "Content/ContentToEngine.h"
#include "Utilities/IOStream.h"
#include <directXTex.h>
#include <dxgi1_6.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace lightning::tools {

	bool is_normal_map(const Image* const image);
	HRESULT equirectangular_to_cubemap(ID3D11Device* device, const Image* env_maps, u32 env_map_count, u32 cubemap_size, bool use_prefilter_size, bool mirror_cubemap, ScratchImage& cubemaps);
	HRESULT equirectangular_to_cubemap(const Image* env_maps, u32 env_map_count, u32 cubemap_size, bool use_prefilter_size, bool mirror_cubemap, ScratchImage& cubemaps);

	namespace {

		struct ImportError {
			enum ErrorCode {
				SUCCEEDED = 0,
				UNKNOWN,
				COMPRESS,
				DECOMPRESS,
				LOAD,
				MIPMAP_GENERATION,
				MAX_SIZE_EXCEEDED,
				SIZE_MISMATCH,
				FORMAT_MISMATCH,
				FILE_NOT_FOUND,
				NEED_SIX_IMAGES,
			};
		};

		struct TextureDimension {
			enum Dimension : u32 {
				TEXTURE_1D,
				TEXTURE_2D,
				TEXTURE_3D,
				TEXTURE_CUBE
			};
		};

		struct TextureImportSettings {
			char* sources;
			u32 source_count;
			u32 dimension;
			u32 mip_levels;
			f32 alpha_threshold;
			u32 prefer_bc7;
			u32 output_format;
			u32 compress;
			u32 cubemap_size;
			u32 mirror_cubemap;
			u32 prefilter_cubemap;
		};

		struct TextureInfo {
			u32 width;
			u32 height;
			u32 array_size;
			u32 mip_levels;
			u32 format;
			u32 import_error;
			u32 flags;
		};

		struct TextureData {
			constexpr static u32 max_mips{ 14 }; // 8k textures;
			u8* subresource_data;
			u32 subresource_size;
			u8* icon;
			u32 icon_size;
			TextureInfo info;
			TextureImportSettings import_settings;
		};

		struct D3D12Device {
			ComPtr<ID3D11Device> device;
			std::mutex hw_compression_mutex;
		};

		std::mutex device_creation_mutex;
		util::vector<D3D12Device> d3d11_devices;

		HMODULE dxgi_module{ nullptr };
		HMODULE d3d11_module{ nullptr };

		util::vector<ComPtr<IDXGIAdapter>> get_adapters_by_performance() {
			if (!dxgi_module) {
				dxgi_module = LoadLibrary("dxgi.dll");

				if (!dxgi_module) return {};
			}

			using PFN_CreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID, void**);

			const PFN_CreateDXGIFactory1 create_dxgi_factory_1{ (PFN_CreateDXGIFactory1)((void*)GetProcAddress(dxgi_module, "CreateDXGIFactory1")) };

			if (!create_dxgi_factory_1) return {};

			ComPtr<IDXGIFactory7> factory;
			util::vector<ComPtr<IDXGIAdapter>> adapters;

			if (SUCCEEDED(create_dxgi_factory_1(IID_PPV_ARGS(factory.GetAddressOf())))) {
				constexpr u32 warp_id{ 0x1414 };

				ComPtr<IDXGIAdapter> adapter;
				for (u32 i{ 0 }; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
					if (!adapter) continue;

					DXGI_ADAPTER_DESC desc;
					adapter->GetDesc(&desc);

					if (desc.VendorId != warp_id) adapters.emplace_back(adapter);

					adapter.Reset();
				}
			}

			return adapters;
		}

		void create_device() {
			if (d3d11_devices.size()) return;

			if (!d3d11_module) {
				d3d11_module = LoadLibrary("d3d11.dll");

				if (!d3d11_module) return;
			}

			const PFN_D3D11_CREATE_DEVICE d3d11_create_device{ (PFN_D3D11_CREATE_DEVICE)((void*)GetProcAddress(d3d11_module, "D3D11CreateDevice")) };
			if (!d3d11_create_device) return;

			u32 create_device_flags{ 0 };
			#ifdef _DEBUG
			create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
			#endif

			util::vector<ComPtr<IDXGIAdapter>> adapters{ get_adapters_by_performance() };
			util::vector<ComPtr<ID3D11Device>> devices(adapters.size(), nullptr);
			constexpr D3D_FEATURE_LEVEL feature_levels[]{ D3D_FEATURE_LEVEL_11_1, };

			for (u32 i{ 0 }; i < adapters.size(); ++i) {
				ID3D11Device** device{ &devices[i] };
				D3D_FEATURE_LEVEL feature_level;
				[[maybe_unused]] HRESULT hr{ d3d11_create_device(adapters[i].Get(), adapters[i] ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags, feature_levels, _countof(feature_levels), D3D11_SDK_VERSION, device, &feature_level, nullptr) };
				assert(SUCCEEDED(hr));
			}

			for (u32 i{ 0 }; i < devices.size(); ++i) {
				if (devices[i]) {
					d3d11_devices.emplace_back();
					d3d11_devices.back().device = devices[i];
				}
			}
		}

		bool try_create_device() {
			std::lock_guard lock{ device_creation_mutex };
			static bool try_once = false;

			if (!try_once) {
				try_once = true;
				create_device();
			}

			return d3d11_devices.size() > 0;
		}

		template<typename T> bool run_on_gpu(T func) {
			if (!try_create_device()) return false;
			bool wait{ true };

			while (wait) {
				for (u32 i{ 0 }; i < d3d11_devices.size(); ++i) {
					if (d3d11_devices[i].hw_compression_mutex.try_lock()) {
						func(d3d11_devices[i].device.Get());
						d3d11_devices[i].hw_compression_mutex.unlock();
						wait = false;
						break;
					}
				}

				if (wait) std::this_thread::sleep_for(std::chrono::milliseconds(200));
			}

			return true;
		}

		constexpr void set_or_clear_flags(u32& flags, u32 flag, bool set) {
			if (set) flags |= flag;
			else flags &= ~flag;
		}

		constexpr u32 get_max_mip_count(u32 width, u32 height, u32 depth) {
			u32 mip_levels{ 1 };

			while (width > 1 || height > 1 || depth > 1) {
				width >>= 1;
				height >>= 1;
				depth >>= 1;

				++mip_levels;
			}

			return mip_levels;
		}

		bool is_hdr(DXGI_FORMAT format) {
			switch (format) {
			case DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT_BC6H_SF16:
			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32_FLOAT:
				return true;
			};

			return false;
		}

		void texture_info_from_metadata(const TexMetadata& metadata, TextureInfo& info) {
			using namespace lightning::content;

			const DXGI_FORMAT format{ metadata.format };
			info.format = format;
			info.width = (u32)metadata.width;
			info.height = (u32)metadata.height;
			info.array_size = metadata.IsVolumemap() ? (u32)metadata.depth : (u32)metadata.arraySize;
			info.mip_levels = (u32)metadata.mipLevels;
			set_or_clear_flags(info.flags, TextureFlags::HAS_ALPHA, HasAlpha(format));
			set_or_clear_flags(info.flags, TextureFlags::IS_HDR, is_hdr(format));
			set_or_clear_flags(info.flags, TextureFlags::IS_CUBE_MAP, metadata.IsCubemap());
			set_or_clear_flags(info.flags, TextureFlags::IS_VOLUME_MAP, metadata.IsVolumemap());
			set_or_clear_flags(info.flags, TextureFlags::IS_SRGB, IsSRGB(format));
		}

		void copy_subresources(const ScratchImage& scratch, TextureData* const data) {
			const Image* const images{ scratch.GetImages() };
			const u32 image_count{ (u32)scratch.GetImageCount() };
			assert(images && scratch.GetMetadata().mipLevels && scratch.GetMetadata().mipLevels <= TextureData::max_mips);

			u64 subresource_size{ 0 };

			for (u32 i{ 0 }; i < image_count; ++i) {
				subresource_size += sizeof(u32) * 4 + images[i].slicePitch;
			}

			if (subresource_size > ~(u32)0) {
				// Support up to 4GB per resource
				data->info.import_error = ImportError::MAX_SIZE_EXCEEDED;
				return;
			}

			data->subresource_size = (u32)subresource_size;
			data->subresource_data = (u8* const)CoTaskMemRealloc(data->subresource_data, subresource_size);
			assert(data->subresource_data);

			util::BlobStreamWriter blob{ data->subresource_data, data->subresource_size };

			for (u32 i{ 0 }; i < image_count; ++i) {
				const Image& image{ images[i] };
				blob.write((u32)image.width);
				blob.write((u32)image.height);
				blob.write((u32)image.rowPitch);
				blob.write((u32)image.slicePitch);
				blob.write(image.pixels, image.slicePitch);
			}
		}

		[[nodiscard]] util::vector<Image> subresource_data_to_images(TextureData* const data) {
			assert(data && data->subresource_data && data->subresource_size);
			assert(data->info.mip_levels && data->info.mip_levels <= TextureData::max_mips);
			assert(data->info.array_size);

			const TextureInfo& info{ data->info };
			u32 image_count{ info.array_size };

			if (info.flags & content::TextureFlags::IS_VOLUME_MAP) {
				u32 depth_per_miplevel{ info.array_size };

				for (u32 i{ 1 }; i < info.mip_levels; ++i) {
					depth_per_miplevel = std::max(depth_per_miplevel >> 1, (u32)1);
					image_count += depth_per_miplevel;
				}
			}
			else {
				image_count *= info.mip_levels;
			}

			util::BlobStreamReader blob{ data->subresource_data };
			util::vector<Image> images(image_count);

			for (u32 i{ 0 }; i < image_count; ++i) {
				Image image{};
				image.width = blob.read<u32>();
				image.height = blob.read<u32>();
				image.format = (DXGI_FORMAT)info.format;
				image.rowPitch = blob.read<u32>();
				image.slicePitch = blob.read<u32>();
				image.pixels = (u8*)blob.position();

				blob.skip(image.slicePitch);

				images[i] = image;
			}

			return images;
		}

		void copy_icon(const Image& bc_image, TextureData* const data) {
			ScratchImage scratch;

			if (FAILED(Decompress(bc_image, DXGI_FORMAT_UNKNOWN, scratch))) {
				return;
			}

			assert(scratch.GetImages());
			const Image& image{ scratch.GetImages()[0] };

			data->icon_size = (u32)(sizeof(u32) * 4 + image.slicePitch);
			data->icon = (u8* const)CoTaskMemRealloc(data->icon, data->icon_size);
			assert(data->icon);
			util::BlobStreamWriter blob{ data->icon, data->icon_size };
			blob.write((u32)image.width);
			blob.write((u32)image.height);
			blob.write((u32)image.rowPitch);
			blob.write((u32)image.slicePitch);
			blob.write(image.pixels, image.slicePitch);
		}

		[[nodiscard]] ScratchImage load_from_file(TextureData* const data, const char* file_name) {
			using namespace lightning::content;

			assert(file_exists(file_name));

			if (!file_exists(file_name)) {
				data->info.import_error = ImportError::FILE_NOT_FOUND;
				return {};
			}

			data->info.import_error = ImportError::LOAD;

			WIC_FLAGS wic_flags{ WIC_FLAGS_NONE };
			TGA_FLAGS tga_flags{ TGA_FLAGS_NONE };

			if (data->import_settings.output_format == DXGI_FORMAT_BC4_UNORM || data->import_settings.output_format == DXGI_FORMAT_BC5_UNORM) {
				wic_flags |= WIC_FLAGS_IGNORE_SRGB;
				tga_flags |= TGA_FLAGS_IGNORE_SRGB;
			}

			const std::wstring w_file{ to_wstring(file_name) };
			const wchar_t* const file{ w_file.c_str() };
			ScratchImage scratch;

			wic_flags |= WIC_FLAGS_FORCE_RGB;
			HRESULT hr{ LoadFromWICFile(file, wic_flags, nullptr, scratch) };

			if (FAILED(hr)) {
				hr = LoadFromTGAFile(file, tga_flags, nullptr, scratch);
			}

			if (FAILED(hr)) {
				hr = LoadFromHDRFile(file, nullptr, scratch);
				if (SUCCEEDED(hr)) data->info.flags |= TextureFlags::IS_HDR;
			}

			if (FAILED(hr)) {
				hr = LoadFromDDSFile(file, DDS_FLAGS_FORCE_RGB, nullptr, scratch);

				if (SUCCEEDED(hr)) {
					data->info.import_error = ImportError::DECOMPRESS;
					ScratchImage mip_scratch;
					hr = Decompress(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), DXGI_FORMAT_UNKNOWN, mip_scratch);

					if (SUCCEEDED(hr)) {
						scratch = std::move(mip_scratch);
					}
				}
			}

			if (SUCCEEDED(hr)) {
				data->info.import_error = ImportError::SUCCEEDED;
			}

			return scratch;
		}

		[[nodiscard]] ScratchImage generate_mipmaps(const ScratchImage& source, TextureInfo& info, u32 mip_levels, bool is_3d) {
			const TexMetadata& metadata{ source.GetMetadata() };
			mip_levels = math::clamp(mip_levels, (u32)0, get_max_mip_count((u32)metadata.width, (u32)metadata.height, (u32)metadata.depth));
			HRESULT hr{ S_OK };

			ScratchImage mip_scratch{};

			if (!is_3d) {
				hr = GenerateMipMaps(source.GetImages(), source.GetImageCount(), source.GetMetadata(), TEX_FILTER_DEFAULT, mip_levels, mip_scratch);
			}
			else {
				hr = GenerateMipMaps3D(source.GetImages(), source.GetImageCount(), source.GetMetadata(), TEX_FILTER_DEFAULT, mip_levels, mip_scratch);
			}

			if (FAILED(hr)) {
				info.import_error = ImportError::MIPMAP_GENERATION;
				return {};
			}

			return mip_scratch;
		}

		[[nodiscard]] ScratchImage initialize_from_images(TextureData* const data, const util::vector<Image>& images) {
			assert(data);
			const TextureImportSettings& settings{ data->import_settings };

			ScratchImage scratch;
			HRESULT hr{ S_OK };
			const u32 array_size{ (u32)images.size() };

			{
				ScratchImage working_scratch{};

				if (settings.dimension == TextureDimension::TEXTURE_1D || settings.dimension == TextureDimension::TEXTURE_2D) {
					const bool allow_1d{ settings.dimension == TextureDimension::TEXTURE_1D };
					assert(array_size >= 1);
					hr = working_scratch.InitializeArrayFromImages(images.data(), images.size(), allow_1d);

				}
				else if (settings.dimension == TextureDimension::TEXTURE_CUBE) {
					const Image& image{ images[0] };
					
					if (math::is_equal((f32)image.width / (f32)image.height, 2.f)) {
						if (!run_on_gpu([&](ID3D11Device* device) {hr = equirectangular_to_cubemap(device, images.data(), array_size, settings.cubemap_size, settings.prefilter_cubemap, settings.mirror_cubemap, working_scratch); })) {
							hr = equirectangular_to_cubemap(images.data(), array_size, settings.cubemap_size, settings.prefilter_cubemap, settings.mirror_cubemap, working_scratch);
						}
					}
					else if (array_size % 6 || image.width != image.height) {
						data->info.import_error = ImportError::NEED_SIX_IMAGES;
						return {};
					}
					else {
						hr = working_scratch.InitializeCubeFromImages(images.data(), images.size());
					}
				}
				else {
					assert(settings.dimension == TextureDimension::TEXTURE_3D);
					hr = working_scratch.Initialize3DFromImages(images.data(), images.size());
				}

				if (FAILED(hr)) {
					data->info.import_error = ImportError::UNKNOWN;
					return{};
				}

				scratch = std::move(working_scratch);
			}

			if (settings.mip_levels != 1 || settings.prefilter_cubemap) {
				scratch = generate_mipmaps(scratch, data->info, settings.prefilter_cubemap ? 0 : settings.mip_levels, settings.dimension == TextureDimension::TEXTURE_3D);
			}

			return scratch;
		}

		DXGI_FORMAT determine_output_format(TextureData* const data, ScratchImage& scratch, const Image* const image) {
			using namespace lightning::content;

			assert(data && data->import_settings.compress);
			const DXGI_FORMAT image_format{ image->format };
			DXGI_FORMAT output_format{ (DXGI_FORMAT)data->import_settings.output_format };

			if (output_format != DXGI_FORMAT_UNKNOWN) {
				goto _done;
			}

			if ((data->info.flags & TextureFlags::IS_HDR) || image_format == DXGI_FORMAT_BC6H_UF16 || image_format == DXGI_FORMAT_BC6H_SF16) {
				output_format = DXGI_FORMAT_BC6H_UF16;
			}
			else if (image_format == DXGI_FORMAT_R8_UNORM || image_format == DXGI_FORMAT_BC4_UNORM || image_format == DXGI_FORMAT_BC4_SNORM) {
				output_format = DXGI_FORMAT_BC4_UNORM;
			}
			else if (is_normal_map(image) || image_format == DXGI_FORMAT_BC5_UNORM || image_format == DXGI_FORMAT_BC5_SNORM) {
				data->info.flags |= TextureFlags::IS_IMPORTED_AS_NORMAL_MAP;
				output_format = DXGI_FORMAT_BC5_UNORM;

				if (IsSRGB(image_format)) {
					scratch.OverrideFormat(MakeTypelessUNORM(MakeTypeless(image_format)));
				}
			}
			else {
				output_format = data->import_settings.prefer_bc7 ? DXGI_FORMAT_BC7_UNORM : scratch.IsAlphaAllOpaque() ? DXGI_FORMAT_BC1_UNORM : DXGI_FORMAT_BC3_UNORM;
			}

		_done:
			assert(IsCompressed(output_format));
			if (HasAlpha((DXGI_FORMAT)output_format)) data->info.flags |= TextureFlags::HAS_ALPHA;

			return IsSRGB(image_format) ? MakeSRGB(output_format) : output_format;
		}

		bool can_use_gpu(DXGI_FORMAT format) {
			switch (format) {
			case DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT_BC6H_SF16:
			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT_BC7_UNORM_SRGB: {
				return true;
			}
			}

			return false;
		}

		[[nodiscard]] ScratchImage compress_image(TextureData* const data, ScratchImage& scratch) {
			assert(data && data->import_settings.compress && scratch.GetImages());
			const Image* const image{ scratch.GetImage(0, 0, 0) };

			if (!image) {
				data->info.import_error = ImportError::UNKNOWN;
				return {};
			}

			const DXGI_FORMAT output_format{ determine_output_format(data, scratch, image) };
			HRESULT hr{ S_OK };
			ScratchImage bc_scratch;

			if (!(can_use_gpu(output_format) && run_on_gpu([&](ID3D11Device* device) { hr = Compress(device, scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), output_format, TEX_COMPRESS_DEFAULT, 1.f, bc_scratch); }))) {
				hr = Compress(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), output_format, TEX_COMPRESS_PARALLEL, data->import_settings.alpha_threshold, bc_scratch);
			}

			if (FAILED(hr)) {
				data->info.import_error = ImportError::COMPRESS;
				return {};
			}

			return bc_scratch;
		}

		[[nodiscard]] ScratchImage decompress_image(TextureData* const data) {
			using namespace lightning::content;
			assert(data->import_settings.compress);
			TextureInfo& info{ data->info };
			const DXGI_FORMAT format{ (DXGI_FORMAT)info.format };
			assert(IsCompressed(format));
			util::vector<Image> images = subresource_data_to_images(data);
			const bool is_3d{ (info.flags & TextureFlags::IS_VOLUME_MAP) != 0 };

			TexMetadata metadata{};
			metadata.width = info.width;
			metadata.height = info.height;
			metadata.depth = is_3d ? info.array_size : 1;
			metadata.arraySize = is_3d ? 1 : info.array_size;
			metadata.mipLevels = info.mip_levels;
			metadata.miscFlags = info.flags & TextureFlags::IS_CUBE_MAP ? TEX_MISC_TEXTURECUBE : 0;
			metadata.miscFlags2 = info.flags & TextureFlags::IS_PREMULTIPLIED_ALPHA ? TEX_ALPHA_MODE_PREMULTIPLIED : info.flags & TextureFlags::HAS_ALPHA ? TEX_ALPHA_MODE_STRAIGHT : TEX_ALPHA_MODE_OPAQUE;
			metadata.format = format;

			metadata.dimension = is_3d ? TEX_DIMENSION_TEXTURE3D : TEX_DIMENSION_TEXTURE2D;

			ScratchImage scratch;
			HRESULT hr{ Decompress(images.data(), (size_t)images.size(), metadata, DXGI_FORMAT_UNKNOWN, scratch) };

			if (FAILED(hr)) {
				data->info.import_error = ImportError::DECOMPRESS;
				return {};
			}

			return scratch;
		}
	}

	void shutdown_texture_tools() {
		d3d11_devices.clear();

		if (dxgi_module) {
			FreeLibrary(dxgi_module);
			dxgi_module = nullptr;
		}

		if (d3d11_module) {
			FreeLibrary(d3d11_module);
			d3d11_module = nullptr;
		}
	}

	EDITOR_INTERFACE void decompress(TextureData* const data) {
		ScratchImage scratch{ decompress_image(data) };

		if (!data->info.import_error) {
			copy_subresources(scratch, data);
			texture_info_from_metadata(scratch.GetMetadata(), data->info);
		}
	}

	EDITOR_INTERFACE void import(TextureData* const data) {
		const TextureImportSettings& settings{ data->import_settings };
		assert(settings.sources&& settings.source_count);

		util::vector<ScratchImage> scratch_images;
		util::vector<Image> images;

		u32 width{ 0 };
		u32 height{ 0 };
		DXGI_FORMAT format{};
		util::vector<std::string> files = split(settings.sources, ';');
		assert(files.size() == settings.source_count);

		for (u32 i{ 0 }; i < settings.source_count; ++i) {
			scratch_images.emplace_back(load_from_file(data, files[i].c_str()));
			if (data->info.import_error) return;

			const ScratchImage& scratch{ scratch_images.back() };
			const TexMetadata& metadata{ scratch.GetMetadata() };

			if (i == 0) {
				width = (u32)metadata.width;
				height = (u32)metadata.height;
				format = metadata.format;
			}

			if (width != metadata.width || height != metadata.height) {
				data->info.import_error = ImportError::SIZE_MISMATCH;
				return;
			}

			if (format != metadata.format) {
				data->info.import_error = ImportError::FORMAT_MISMATCH;
				return;
			}

			const u32 array_size{ (u32)metadata.arraySize };
			const u32 depth{ (u32)metadata.depth };

			for (u32 array_index{ 0 }; array_index < array_size; ++array_index) {
				for (u32 depth_index{ 0 }; depth_index < depth; ++depth_index) {
					const Image* image{ scratch.GetImage(0, array_index, depth_index) };
					assert(image);

					if (!image) {
						data->info.import_error = ImportError::UNKNOWN;
						return;
					}

					if (width != image->width || height != image->height) {
						data->info.import_error = ImportError::SIZE_MISMATCH;
						return;
					}

					images.emplace_back(*image);
				}
			}
		}

		ScratchImage scratch{ initialize_from_images(data, images) };

		if (data->info.import_error) return;

		if (settings.compress) {
			ScratchImage bc_scratch{};

			if (data->info.import_error) return;

			assert(bc_scratch.GetImages());
			copy_icon(bc_scratch.GetImages()[0], data);
			
			scratch = std::move(bc_scratch);
		}

		copy_subresources(scratch, data);
		texture_info_from_metadata(scratch.GetMetadata(), data->info);
	}
}