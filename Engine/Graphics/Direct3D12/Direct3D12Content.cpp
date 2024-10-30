#include "Direct3D12Content.h"
#include "Direct3D12Core.h"
#include "Utilities/IOStream.h"
#include "Content/ContentToEngine.h"
#include "Direct3D12GPass.h"
#include "Direct3D12Upload.h"

#ifdef OPAQUE
#undef OPAQUE
#endif

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif

namespace lightning::graphics::direct3d12::content {
	namespace {

		struct PsoId {
			id::id_type gpass_pso_id{ id::invalid_id };
			id::id_type depth_pso_id{ id::invalid_id };
		};

		struct SubmeshView {
			D3D12_VERTEX_BUFFER_VIEW position_buffer_view{};
			D3D12_VERTEX_BUFFER_VIEW element_buffer_view{};
			D3D12_INDEX_BUFFER_VIEW index_buffer_view{};
			D3D_PRIMITIVE_TOPOLOGY primitive_topology{};
			u32 element_type{};
		};

		struct D3D12RenderItem {
			id::id_type entity_id;
			id::id_type submesh_gpu_id;
			id::id_type material_id;
			id::id_type pso_id;
			id::id_type depth_pso_id;
		};

		util::free_list<ID3D12Resource*> submesh_buffers{};
		util::free_list<SubmeshView> submesh_views{};
		std::mutex submesh_mutex{};

		util::free_list<D3D12Texture> textures;
		util::free_list<u32> descriptor_indicies;
		std::mutex texture_mutex{};

		util::vector<ID3D12RootSignature*> root_signatures;
		std::unordered_map<u64, id::id_type> material_rs_map;
		util::free_list<std::unique_ptr<u8[]>> materials;
		std::mutex material_mutex{};

		util::free_list<D3D12RenderItem> render_items;
		util::free_list<std::unique_ptr<id::id_type[]>> render_item_ids;
		std::mutex render_item_mutex{};

		util::vector<ID3D12PipelineState*> pipeline_states;
		std::unordered_map<u64, id::id_type> pso_map;
		std::mutex pso_mutex{};

		struct {
			util::vector<lightning::content::LodOffset> lod_offsets;
			util::vector<id::id_type> geometry_ids;
		} frame_cache;

		constexpr D3D12_ROOT_SIGNATURE_FLAGS get_root_signature_flags(ShaderFlags::Flags flags) {
			D3D12_ROOT_SIGNATURE_FLAGS default_flags{ d3dx::D3D12RootSignatureDesc::default_flags };

			if (flags & ShaderFlags::VERTEX) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::HULL) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::DOMAIN) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::GEOMETRY) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::PIXEL) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::AMPLIFICATION) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
			if (flags & ShaderFlags::MESH) default_flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

			return default_flags;
		}

		id::id_type create_root_signature(MaterialType::Type type, ShaderFlags::Flags flags) {
			assert(type < MaterialType::count);
			static_assert(sizeof(type) == sizeof(u32) && sizeof(flags) == sizeof(u32));
			const u64 key{ ((u64)type << 32) | flags };
			auto pair = material_rs_map.find(key);

			if (pair != material_rs_map.end()) {
				assert(pair->first == key);
				return pair->second;
			}

			ID3D12RootSignature* root_signature{ nullptr };

			switch (type) {
				case MaterialType::Type::OPAQUE:
					using params = gpass::OpaqueRootParameter;
					d3dx::D3D12RootParameter parameters[params::count]{};

					D3D12_SHADER_VISIBILITY buffer_visibility{};
					D3D12_SHADER_VISIBILITY data_visibility{};

					if (flags & ShaderFlags::VERTEX) {
						buffer_visibility = D3D12_SHADER_VISIBILITY_VERTEX;
						data_visibility = D3D12_SHADER_VISIBILITY_VERTEX;
					}
					else if (flags & ShaderFlags::MESH) {
						buffer_visibility = D3D12_SHADER_VISIBILITY_MESH;
						data_visibility = D3D12_SHADER_VISIBILITY_MESH;
					}

					if ((flags & ShaderFlags::HULL) || (flags & ShaderFlags::GEOMETRY) || (flags & ShaderFlags::AMPLIFICATION)) {
						buffer_visibility = D3D12_SHADER_VISIBILITY_ALL;
						data_visibility = D3D12_SHADER_VISIBILITY_ALL;
					}

					if ((flags & ShaderFlags::PIXEL) || (flags & ShaderFlags::COMPUTE)) {
						data_visibility = D3D12_SHADER_VISIBILITY_ALL;
					}

					parameters[params::GLOBAL_SHADER_DATA].as_cbv(D3D12_SHADER_VISIBILITY_ALL, 0);
					parameters[params::PER_OBJECT_DATA].as_cbv(data_visibility, 1);
					parameters[params::POSITION_BUFFER].as_srv(buffer_visibility, 0);
					parameters[params::ELEMENT_BUFFER].as_srv(buffer_visibility, 1);
					parameters[params::SRV_INDICIES].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 2);
					parameters[params::DIRECTIONAL_LIGHTS].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 3);
					parameters[params::CULLABLE_LIGHTS].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 4);
					parameters[params::LIGHT_GRID].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 5);
					parameters[params::LIGHT_INDEX_LIST].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 6);

					const D3D12_STATIC_SAMPLER_DESC samplers[]{
						d3dx::static_sampler(d3dx::sampler_state.static_point, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL),
						d3dx::static_sampler(d3dx::sampler_state.static_linear, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL),
						d3dx::static_sampler(d3dx::sampler_state.static_anisotropic, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL),
					};

					root_signature = d3dx::D3D12RootSignatureDesc {
						&parameters[0], _countof(parameters), get_root_signature_flags(flags),
						&samplers[0], _countof(samplers)
					}.create();

					break;
			}

			assert(root_signature);
			const id::id_type id{ (id::id_type)root_signatures.size() };
			root_signatures.emplace_back(root_signature);
			material_rs_map[key] = id;
			NAME_D3D12_OBJECT_INDEXED(root_signature, key, L"GPass Root Signature - key");

			return id;
		}

		class D3D12MaterialStream {
			public:
				DISABLE_COPY_AND_MOVE(D3D12MaterialStream);
				explicit D3D12MaterialStream(u8* const material_buffer) : _buffer{ material_buffer } {
					initialize();
				}

				explicit D3D12MaterialStream(std::unique_ptr<u8[]>& material_buffer, MaterialInitInfo info) {
					assert(!material_buffer);

					u32 shader_count{ 0 };
					u32 flags{ 0 };
					for (u32 i{ 0 }; i < ShaderType::count; ++i) {
						if (id::is_valid(info.shader_ids[i])) {
							++shader_count;
							flags |= (1 << i);
						}
					}

					assert(shader_count && flags);

					const u32 buffer_size{
						sizeof(MaterialType::Type) +								// material type
						sizeof(ShaderFlags::Flags) +								// shader flags
						sizeof(id::id_type) +										// root signature id
						sizeof(u32) +												// texture count
						sizeof(MaterialSurface) +										// PBR material properties
						sizeof(id::id_type) * shader_count +						// shader ids
						(sizeof(id::id_type) + sizeof(u32)) * info.texture_count	// texture ids and descriptor indicies 
					};

					material_buffer = std::make_unique<u8[]>(buffer_size);
					_buffer = material_buffer.get();
					u8* const buffer{ _buffer };

					*(MaterialType::Type*)buffer = info.type;
					*(ShaderFlags::Flags*)(&buffer[shader_flags_index]) = (ShaderFlags::Flags)flags;
					*(id::id_type*)(&buffer[root_signature_index]) = create_root_signature(info.type, (ShaderFlags::Flags)flags);
					*(u32*)(&buffer[texture_count_index]) = info.texture_count;
					*(MaterialSurface*)&buffer[material_surface_index] = info.surface;

					initialize();

					if (info.texture_count) {
						memcpy(_texture_ids, info.texture_ids, info.texture_count * sizeof(id::id_type));
						texture::get_descriptor_indicies(_texture_ids, info.texture_count, _descriptor_indicies);
					}

					u32 shader_index{ 0 };
					for (u32 i{ 0 }; i < ShaderType::count; ++i) {
						if (id::is_valid(info.shader_ids[i])) {
							_shader_ids[shader_index] = info.shader_ids[i];
							++shader_index;
						}
					}

					assert(shader_index == (u32)_mm_popcnt_u32(_shader_flags));
 				}

				[[nodiscard]] constexpr u32 texture_count() const { return _texture_count; }
				[[nodiscard]] constexpr MaterialType::Type material_type() const { return _type; }
				[[nodiscard]] constexpr ShaderFlags::Flags shader_flags() const { return _shader_flags; }
				[[nodiscard]] constexpr id::id_type root_signature_id() const { return _root_signature_id; }
				[[nodiscard]] constexpr id::id_type* texture_ids() const { return _texture_ids; }
				[[nodiscard]] constexpr u32* descriptor_indicies() const { return _descriptor_indicies; }
				[[nodiscard]] constexpr id::id_type* shader_ids() const { return _shader_ids; }
				[[nodiscard]] constexpr MaterialSurface* surface() const { return _material_surface; }

			private:
				void initialize() {
					assert(_buffer);

					u8* const buffer{ _buffer };

					_type = *(MaterialType::Type*)buffer;
					_shader_flags = *(ShaderFlags::Flags*)&buffer[shader_flags_index];
					_root_signature_id = *(id::id_type*)&buffer[root_signature_index];
					_texture_count = *(u32*)&buffer[texture_count_index];
					_material_surface = (MaterialSurface*)&buffer[material_surface_index];

					_shader_ids = (id::id_type*)&buffer[material_surface_index + sizeof(MaterialSurface)];
					_texture_ids = _texture_count ? &_shader_ids[_mm_popcnt_u32(_shader_flags)] : nullptr;
					_descriptor_indicies = _texture_count ? (u32*)&_texture_ids[_texture_count] : nullptr;
				}

				constexpr static u32 shader_flags_index{ sizeof(MaterialType::Type) };
				constexpr static u32 root_signature_index{ shader_flags_index + sizeof(ShaderFlags::Flags) };
				constexpr static u32 texture_count_index{ root_signature_index + sizeof(id::id_type) };
				constexpr static u32 material_surface_index{ texture_count_index + sizeof(u32) };

				u8* _buffer;
				MaterialSurface* _material_surface;
				id::id_type* _texture_ids;
				u32* _descriptor_indicies;
				id::id_type* _shader_ids;
				id::id_type _root_signature_id;
				u32 _texture_count;
				MaterialType::Type _type;
				ShaderFlags::Flags _shader_flags;
		};

		constexpr D3D_PRIMITIVE_TOPOLOGY get_d3d_primitive_topology(PrimitiveTopology::Type type) {

			assert(type < PrimitiveTopology::count);

			switch (type) {
				case PrimitiveTopology::Type::POINT_LIST: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
				case PrimitiveTopology::Type::LINE_LIST: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
				case PrimitiveTopology::Type::LINE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
				case PrimitiveTopology::Type::TRIANGLE_LIST: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				case PrimitiveTopology::Type::TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			}
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

		constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE get_d3d_primitive_topology_type(D3D_PRIMITIVE_TOPOLOGY topology) {

			switch (topology) {
				case D3D_PRIMITIVE_TOPOLOGY_POINTLIST: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
				case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
				case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
				case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
				case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			}
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}

		id::id_type create_pso_if_needed(const u8* const stream_ptr, u64 aligned_stream_size, [[maybe_unused]] bool is_depth) {
			const u64 key{ math::calc_crc32_u64(stream_ptr, aligned_stream_size) };

			{
				std::lock_guard lock{ pso_mutex };
				auto pair = pso_map.find(key);

				if (pair != pso_map.end()) {
					assert(pair->first == key);
					return pair->second;
				}
			}

			d3dx::D3D12PipelineStateSubobjectStream* const stream{ (d3dx::D3D12PipelineStateSubobjectStream* const)stream_ptr };
			ID3D12PipelineState* pso{ d3dx::create_pipeline_state(stream, sizeof(d3dx::D3D12PipelineStateSubobjectStream)) };

			{
				std::lock_guard lock{ pso_mutex };
				const id::id_type id{ (u32)pipeline_states.size() };
				pipeline_states.emplace_back(pso);

				NAME_D3D12_OBJECT_INDEXED(pipeline_states.back(), key, is_depth ? L"Depth-only Pipeline State Object - key" : L"GPass Pipeline State Object - key");

				pso_map[key] = id;

				return id;
			}
		}

		#pragma intrinsic(_BitScanForward)
		ShaderType::Type get_shader_type(u32 flag) {
			assert(flag);
			unsigned long index;
			_BitScanForward(&index, flag);
			return (ShaderType::Type)index;
		}

		PsoId create_pso(id::id_type material_id, D3D12_PRIMITIVE_TOPOLOGY primitive_topology, u32 elements_type) {
			constexpr u64 aligned_stream_size{ math::align_size_up<sizeof(u64)>(sizeof(d3dx::D3D12PipelineStateSubobjectStream)) };
			u8* const stream_ptr{ (u8* const)alloca(aligned_stream_size) };
			ZeroMemory(stream_ptr, aligned_stream_size);
			new (stream_ptr) d3dx::D3D12PipelineStateSubobjectStream{};

			d3dx::D3D12PipelineStateSubobjectStream& stream{ *(d3dx::D3D12PipelineStateSubobjectStream* const)stream_ptr };

			{
				std::lock_guard lock{ material_mutex };
				const D3D12MaterialStream material{ materials[material_id].get() };

				D3D12_RT_FORMAT_ARRAY rt_array{};
				rt_array.NumRenderTargets = 1;
				rt_array.RTFormats[0] = gpass::main_buffer_format;

				stream.render_target_formats = rt_array;
				stream.root_signature = root_signatures[material.root_signature_id()];
				stream.primitive_topology = get_d3d_primitive_topology_type(primitive_topology);
				stream.depth_stencil_format = gpass::depth_buffer_format;
				stream.rasterizer = d3dx::rasterizer_state.backface_cull;
				stream.depth_stencil1 = d3dx::depth_state.reversed_readonly;
				stream.blend = d3dx::blend_state.disabled;

				const ShaderFlags::Flags flags{ material.shader_flags() };
				D3D12_SHADER_BYTECODE shaders[ShaderType::count]{};
				u32 shader_index{ 0 };
				for (u32 i{ 0 }; i < ShaderType::count; ++i) {
					if (flags & (1 << i)) {
						const u32 key{ get_shader_type(flags & (1 << i)) == ShaderType::VERTEX ? elements_type : u32_invalid_id };

						lightning::content::compiled_shader_ptr shader{ lightning::content::get_shader(material.shader_ids()[shader_index], key) };
						assert(shader);
						shaders[i].pShaderBytecode = shader->byte_code();
						shaders[i].BytecodeLength = shader->byte_code_size();
						++shader_index;
					}
				}

				stream.vs = shaders[ShaderType::VERTEX];
				stream.ds = shaders[ShaderType::DOMAIN];
				stream.hs = shaders[ShaderType::HULL];
				stream.gs = shaders[ShaderType::GEOMETRY];
				stream.ps = shaders[ShaderType::PIXEL];
				stream.cs = shaders[ShaderType::COMPUTE];
				stream.as = shaders[ShaderType::AMPLIFICATION];
				stream.ms = shaders[ShaderType::MESH];
			}

			PsoId id_pair{};
			id_pair.gpass_pso_id = create_pso_if_needed(stream_ptr, aligned_stream_size, false);

			stream.ps = D3D12_SHADER_BYTECODE{};
			stream.depth_stencil1 = d3dx::depth_state.reversed;
			id_pair.depth_pso_id = create_pso_if_needed(stream_ptr, aligned_stream_size, true);

			return id_pair;
		}

		D3D12Texture create_resource_from_texture_data(const u8* const data) {
			assert(data);
			util::BlobStreamReader blob{ data };
			const u32 width{ blob.read<u32>() };
			const u32 height{ blob.read<u32>() };
			u32 depth{ 1 };
			u32 array_size{ blob.read<u32>() };
			const u32 flags{ blob.read<u32>() };
			const u32 mip_levels{ blob.read<u32>() };
			const DXGI_FORMAT format{ (DXGI_FORMAT)blob.read<u32>() };
			const bool is_3d{ (flags & lightning::content::TextureFlags::IS_VOLUME_MAP) != 0 };
			
			assert(mip_levels <= D3D12Texture::max_mips);
			u32 depth_per_mip_level[D3D12Texture::max_mips]{};

			for (u32 i{ 0 }; i < D3D12Texture::max_mips; ++i) {
				depth_per_mip_level[i] = 1;
			}

			if (is_3d) {
				depth = array_size;
				array_size = 1;
				u32 depth_per_mip{ depth };

				for (u32 i{ 0 }; i < mip_levels; ++i) {
					depth_per_mip_level[i] = depth_per_mip;
					depth_per_mip = std::max(depth_per_mip >> 1, (u32)1);
				}
			}

			util::vector<D3D12_SUBRESOURCE_DATA> subresources{};

			for (u32 i{ 0 }; i < array_size; ++i) {
				for (u32 j{ 0 }; j < mip_levels; ++j) {
					const u32 row_pitch{ blob.read<u32>() };
					const u32 slice_pitch{ blob.read<u32>() };

					subresources.emplace_back(D3D12_SUBRESOURCE_DATA{ blob.position(), row_pitch, slice_pitch });
					blob.skip(slice_pitch * depth_per_mip_level[j]);
				}
			}

			D3D12_RESOURCE_DESC desc{};
			desc.Dimension = is_3d ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Alignment = 0;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = is_3d ? (u16)depth : (u16)array_size;
			desc.MipLevels = (u16)mip_levels;
			desc.Format = format;
			desc.SampleDesc = { 1, 0 };
			desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;

			assert(!(flags & lightning::content::TextureFlags::IS_CUBE_MAP && (array_size % 6)));
			const u32 subresource_count{ array_size * mip_levels };
			assert(subresource_count);

			const u32 footprints_data_size{ (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(u32) + sizeof(u64)) * subresource_count };
			std::unique_ptr<u8[]> footprints_data{ std::make_unique<u8[]>(footprints_data_size) };

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* const layouts{ (D3D12_PLACED_SUBRESOURCE_FOOTPRINT* const)footprints_data.get() };
			u32* const num_rows{ (u32* const)&layouts[subresource_count] };
			u64* const row_sizes{ (u64* const)&num_rows[subresource_count] };
			u64 required_size{ 0 };
			id3d12_device* device{ core::device() };

			device->GetCopyableFootprints(&desc, 0, subresource_count, 0, layouts, num_rows, row_sizes, &required_size);

			assert(required_size);
			upload::D3D12UploadContext context{ (u32)required_size };
			u8* const cpu_address{ (u8* const)context.cpu_address() };

			for (u32 subresource_idx{ 0 }; subresource_idx < subresource_count; ++subresource_idx) {
				const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout{ layouts[subresource_idx] };
				const u32 subresource_height{ num_rows[subresource_idx] };
				const u32 subresource_depth{ layout.Footprint.Depth };
				const D3D12_SUBRESOURCE_DATA& subresource{ subresources[subresource_idx] };

				const D3D12_MEMCPY_DEST copy_dst{
					cpu_address + layout.Offset,
					layout.Footprint.RowPitch,
					layout.Footprint.RowPitch * subresource_height
				};

				for (u32 depth_idx{ 0 }; depth_idx < subresource_depth; ++depth_idx) {
					u8* const src_slice{ (u8* const)subresource.pData + subresource.SlicePitch * depth_idx };
					u8* const dst_slice{ (u8* const)copy_dst.pData + copy_dst.SlicePitch * depth_idx };

					for (u32 row_idx{ 0 }; row_idx < subresource_height; ++row_idx) {
						memcpy(dst_slice + copy_dst.RowPitch * row_idx, src_slice + subresource.RowPitch * row_idx, row_sizes[subresource_idx]);
					}
				}
			}

			ID3D12Resource2* resource{ nullptr };
			DXCall(device->CreateCommittedResource(&d3dx::heap_properties.default_heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)));
			ID3D12Resource* upload_buffer{ context.upload_buffer() };

			for (u32 i{ 0 }; i < subresource_count; ++i) {
				D3D12_TEXTURE_COPY_LOCATION src{};
				src.pResource = upload_buffer;
				src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				src.PlacedFootprint = layouts[i];

				D3D12_TEXTURE_COPY_LOCATION dst{};
				dst.pResource = resource;
				dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dst.SubresourceIndex = i;

				context.command_list()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
			}

			context.end_upload();
			assert(resource);
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
			D3D12TextureInitInfo info{};
			info.resource = resource;

			if (flags & lightning::content::TextureFlags::IS_CUBE_MAP) {
				assert(array_size % 6);
				srv_desc.Format = format;
				srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				if (array_size > 6) {
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					srv_desc.TextureCubeArray.MostDetailedMip = 0;
					srv_desc.TextureCubeArray.MipLevels = mip_levels;
					srv_desc.TextureCubeArray.NumCubes = array_size / 6;
				}
				else {
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					srv_desc.TextureCube.MostDetailedMip = 0;
					srv_desc.TextureCube.MipLevels = mip_levels;
					srv_desc.TextureCube.ResourceMinLODClamp = .0f;
				}

				info.srv_desc = &srv_desc;
			}

			return D3D12Texture{ info };
		}
	}

	bool initialize() { return true; }

	void shutdown() {
		for (auto& item : root_signatures) {
			core::release(item);
		}

		material_rs_map.clear();
		root_signatures.clear();

		for (auto& item : pipeline_states) {
			core::release(item);
		}

		pso_map.clear();
		pipeline_states.clear();
	}

	namespace submesh {
		id::id_type add(const u8*& data) {
			util::BlobStreamReader blob{ (const u8*)data };

			const u32 element_size{ blob.read<u32>()};
			const u32 vertex_count{ blob.read<u32>() };
			const u32 index_count{ blob.read<u32>() };
			const u32 elements_type{ blob.read<u32>() };
			const u32 primitive_topology{ blob.read<u32>() };
			const u32 index_size{ (vertex_count < (1 << 16)) ? sizeof(u16) : sizeof(u32) };

			const u32 position_buffer_size{ sizeof(math::v3) * vertex_count };
			const u32 element_buffer_size{ element_size * vertex_count };
			const u32 index_buffer_size{ index_size * index_count };

			constexpr u32 alignment{ D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE };
			const u32 aligned_position_buffer_size{ (u32)math::align_size_up<alignment>(position_buffer_size) };
			const u32 aligned_element_buffer_size{ (u32)math::align_size_up<alignment>(element_buffer_size) };
			const u32 total_buffer_size{ aligned_position_buffer_size + aligned_element_buffer_size + index_buffer_size };

			ID3D12Resource* resource{ d3dx::create_buffer(blob.position(), total_buffer_size) };

			blob.skip(total_buffer_size);
			data = blob.position();

			SubmeshView view{};
			view.position_buffer_view.BufferLocation = resource->GetGPUVirtualAddress();
			view.position_buffer_view.SizeInBytes = position_buffer_size;
			view.position_buffer_view.StrideInBytes = sizeof(math::v3);

			if (element_size) {
				view.element_buffer_view.BufferLocation = resource->GetGPUVirtualAddress() + aligned_position_buffer_size;
				view.element_buffer_view.SizeInBytes = element_buffer_size;
				view.element_buffer_view.StrideInBytes = element_size;
			}

			view.index_buffer_view.BufferLocation = resource->GetGPUVirtualAddress() + aligned_position_buffer_size + aligned_element_buffer_size;
			view.index_buffer_view.Format = (index_size == sizeof(u16)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			view.index_buffer_view.SizeInBytes = index_buffer_size;

			view.element_type = elements_type;
			view.primitive_topology = get_d3d_primitive_topology((PrimitiveTopology::Type)primitive_topology);

			std::lock_guard lock{ submesh_mutex };
			submesh_buffers.add(resource);
			return submesh_views.add(view);
		} 

		void remove(id::id_type id) {
			std::lock_guard lock{ submesh_mutex };
			submesh_views.remove(id);

			core::deferred_release(submesh_buffers[id]);
			submesh_buffers.remove(id);
		}

		void get_views(const id::id_type* const gpu_ids, u32 id_count, const ViewsCache& cache) {
			assert(gpu_ids && id_count);
			assert(cache.position_buffers && cache.element_buffers && cache.index_buffer_views && cache.primitive_topologies && cache.elements_types);

			std::lock_guard lock{ submesh_mutex };

			for (u32 i{ 0 }; i < id_count; ++i) {
				const SubmeshView& view{ submesh_views[gpu_ids[i]] };
				cache.position_buffers[i] = view.position_buffer_view.BufferLocation;
				cache.element_buffers[i] = view.element_buffer_view.BufferLocation;
				cache.index_buffer_views[i] = view.index_buffer_view;
				cache.primitive_topologies[i] = view.primitive_topology;
				cache.elements_types[i] = view.element_type;
			}
		}
	}

	namespace texture {
		id::id_type add(const u8* const data) {
			assert(data);
			D3D12Texture texture{ create_resource_from_texture_data(data) };

			std::lock_guard lock{ texture_mutex };
			const id::id_type id{ textures.add(std::move(texture)) };
			descriptor_indicies.add(textures[id].srv().index);

			return id;
		}

		void remove(id::id_type id) {
			std::lock_guard lock{ texture_mutex };
			textures.remove(id);
			descriptor_indicies.remove(id);
		}

		void get_descriptor_indicies(const id::id_type* const texture_ids, u32 id_count, u32* const indicies) {
			assert(texture_ids && id_count && indicies);

			std::lock_guard lock{ texture_mutex };

			for (u32 i{ 0 }; i < id_count; ++i) {
				assert(id::is_valid(texture_ids[i]));
				indicies[i] = descriptor_indicies[texture_ids[i]];
			}
		}
	}

	namespace material {
		id::id_type add(MaterialInitInfo info) {
			std::unique_ptr<u8[]> buffer;
			std::lock_guard lock{ material_mutex };

			D3D12MaterialStream stream{ buffer, info };

			assert(buffer);
			return materials.add(std::move(buffer));
		}

		void remove(id::id_type id) {
			std::lock_guard lock{ material_mutex };
			materials.remove(id);
		}

		void get_materials(const id::id_type* const material_ids, u32 material_count, const MaterialsCache& cache, u32& descriptor_index_count) {
			assert(material_ids && material_count);
			assert(cache.root_signatures && cache.material_types);

			std::lock_guard lock{ material_mutex };

			u32 total_index_count{ 0 };

			for (u32 i{ 0 }; i < material_count; ++i) {
				const D3D12MaterialStream stream{ materials[material_ids[i]].get() };
				cache.root_signatures[i] = root_signatures[stream.root_signature_id()];
				cache.material_types[i] = stream.material_type();
				cache.descriptor_indices[i] = stream.descriptor_indicies();
				cache.texture_count[i] = stream.texture_count();
				cache.material_surfaces[i] = stream.surface();
				total_index_count += stream.texture_count();
			}

			descriptor_index_count = total_index_count;
		}
	}

	namespace render_item {
		id::id_type add(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count, const id::id_type* const material_ids) {
			assert(id::is_valid(entity_id) && id::is_valid(geometry_content_id));
			assert(material_count && material_ids);

			id::id_type* const gpu_ids{ (id::id_type* const)alloca(material_count * sizeof(id::id_type)) };
			lightning::content::get_submesh_gpu_ids(geometry_content_id, material_count, gpu_ids);

			submesh::ViewsCache views_cache{
				(D3D12_GPU_VIRTUAL_ADDRESS* const)alloca(material_count * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
				(D3D12_GPU_VIRTUAL_ADDRESS* const)alloca(material_count * sizeof(D3D12_GPU_VIRTUAL_ADDRESS)),
				(D3D12_INDEX_BUFFER_VIEW* const)alloca(material_count * sizeof(D3D12_INDEX_BUFFER_VIEW)),
				(D3D_PRIMITIVE_TOPOLOGY* const)alloca(material_count * sizeof(D3D_PRIMITIVE_TOPOLOGY)),
				(u32* const)alloca(material_count * sizeof(u32))
			};

			submesh::get_views(gpu_ids, material_count, views_cache);

			std::unique_ptr<id::id_type[]> items{ std::make_unique<id::id_type[]>(sizeof(id::id_type) * (1 + (u64)material_count + 1)) };

			items[0] = geometry_content_id;
			id::id_type* const item_ids{ &items[1] };

			D3D12RenderItem* const d3d12_items{ (D3D12RenderItem* const)alloca(material_count * sizeof(D3D12RenderItem)) };

			for (u32 i{ 0 }; i < material_count; ++i) {
				D3D12RenderItem& item{ d3d12_items[i] };
				item.entity_id = entity_id;
				item.submesh_gpu_id = gpu_ids[i];
				item.material_id = material_ids[i];
				PsoId id_pair{ create_pso(item.material_id, views_cache.primitive_topologies[i], views_cache.elements_types[i]) };
				item.pso_id = id_pair.gpass_pso_id;
				item.depth_pso_id = id_pair.depth_pso_id;

				assert(id::is_valid(item.submesh_gpu_id) && id::is_valid(item.material_id)); 
			}
			
			std::lock_guard lock{ render_item_mutex };

			for (u32 i{ 0 }; i < material_count; ++i) {
				item_ids[i] = render_items.add(d3d12_items[i]);
			}

			item_ids[material_count] = id::invalid_id;

			return render_item_ids.add(std::move(items));
		}

		void remove(id::id_type id) {
			std::lock_guard lock{ render_item_mutex };

			const id::id_type* const item_ids{ &render_item_ids[id][1] };

			for (u32 i{ 0 }; item_ids[i] != id::invalid_id; ++i) {
				render_items.remove(item_ids[i]);
			}

			render_item_ids.remove(id);
		}

		void get_d3d12_render_items_id(const FrameInfo& info, util::vector<id::id_type>& d3d12_render_item_ids) {
			assert(info.render_item_ids && info.thresholds && info.render_item_count);
			assert(d3d12_render_item_ids.empty());

			frame_cache.lod_offsets.clear();
			frame_cache.geometry_ids.clear();
			const u32 count{ info.render_item_count };

			std::lock_guard lock{ render_item_mutex };

			for (u32 i{ 0 }; i < count; ++i) {
				const id::id_type* const buffer{ render_item_ids[info.render_item_ids[i]].get() };
				frame_cache.geometry_ids.emplace_back(buffer[0]);
			}

			lightning::content::get_lod_offsets(frame_cache.geometry_ids.data(), info.thresholds, count, frame_cache.lod_offsets);

			assert(frame_cache.lod_offsets.size() == count);

			u32 d3d12_render_item_count{ 0 };

			for (u32 i{ 0 }; i < count; ++i) {
				d3d12_render_item_count += frame_cache.lod_offsets[i].count;
			}

			assert(d3d12_render_item_count);

			d3d12_render_item_ids.resize(d3d12_render_item_count);
			u32 item_index{ 0 };

			for (u32 i{ 0 }; i < count; ++i) {
				const id::id_type* const item_ids{ &render_item_ids[info.render_item_ids[i]][1] };
				const lightning::content::LodOffset& lod_offset{ frame_cache.lod_offsets[i] };
				memcpy(&d3d12_render_item_ids[item_index], &item_ids[lod_offset.offset], sizeof(id::id_type) * lod_offset.count);
				item_index += lod_offset.count;

				assert(item_index <= d3d12_render_item_count);
			}
			assert(item_index == d3d12_render_item_count);
		}

		void get_items(const id::id_type* const d3d12_render_item_ids, u32 id_count, const ItemsCache& cache) {
			assert(d3d12_render_item_ids && id_count);
			assert(cache.entity_ids && cache.submesh_gpu_ids && cache.material_ids && cache.gpass_psos && cache.depth_psos);

			std::lock_guard lock_1{ render_item_mutex };
			std::lock_guard lock_2{ pso_mutex };

			for (u32 i{ 0 }; i < id_count; ++i) {
				const D3D12RenderItem& item{ render_items[d3d12_render_item_ids[i]] };
				cache.entity_ids[i] = item.entity_id;
				cache.submesh_gpu_ids[i] = item.submesh_gpu_id;
				cache.material_ids[i] = item.material_id;
				cache.gpass_psos[i] = pipeline_states[item.pso_id];
				cache.depth_psos[i] = pipeline_states[item.depth_pso_id];
			}
		}
	}
}