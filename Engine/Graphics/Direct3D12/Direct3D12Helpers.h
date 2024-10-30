#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12::d3dx {
	constexpr struct {
		const D3D12_HEAP_PROPERTIES default_heap{
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0
		};

		const D3D12_HEAP_PROPERTIES upload_heap{
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,
			0
		};
	} heap_properties;

	constexpr struct {
		const D3D12_RASTERIZER_DESC no_cull{
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_NONE,
			1,
			0,
			0,
			0,
			1,
			0,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		const D3D12_RASTERIZER_DESC backface_cull{
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK,
			1,
			0,
			0,
			0,
			1,
			0,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		const D3D12_RASTERIZER_DESC frontface_cull{
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_FRONT,
			1,
			0,
			0,
			0,
			1,
			0,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		const D3D12_RASTERIZER_DESC wireframe{
			D3D12_FILL_MODE_WIREFRAME,
			D3D12_CULL_MODE_NONE,
			1,
			0,
			0,
			0,
			1,
			0,
			0,
			0,
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

	} rasterizer_state;

	constexpr struct {
		const D3D12_DEPTH_STENCIL_DESC1 disabled{
			0,
			D3D12_DEPTH_WRITE_MASK_ZERO,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			0,
			0,
			0,
			{},
			{},
			0
		};

		const D3D12_DEPTH_STENCIL_DESC1 enabled{
			1,
			D3D12_DEPTH_WRITE_MASK_ALL,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			0,
			0,
			0,
			{},
			{},
			0
		};

		const D3D12_DEPTH_STENCIL_DESC1 enabled_readonly{
			1,
			D3D12_DEPTH_WRITE_MASK_ZERO,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			0,
			0,
			0,
			{},
			{},
			0
		};

		const D3D12_DEPTH_STENCIL_DESC1 reversed{
			1,
			D3D12_DEPTH_WRITE_MASK_ALL,
			D3D12_COMPARISON_FUNC_GREATER_EQUAL,
			0,
			0,
			0,
			{},
			{},
			0
		};

		const D3D12_DEPTH_STENCIL_DESC1 reversed_readonly{
			1,
			D3D12_DEPTH_WRITE_MASK_ZERO,
			D3D12_COMPARISON_FUNC_GREATER_EQUAL,
			0,
			0,
			0,
			{},
			{},
			0
		};

	} depth_state;

	constexpr struct {
		const D3D12_BLEND_DESC disabled{
			0,										// Alpha to coverage enable
			0,										// Independent blend enable
			{
				{
					0,								// Blend Enable
					0,								// Logic OP enable
					D3D12_BLEND_SRC_ALPHA,			// Src blend
					D3D12_BLEND_INV_SRC_ALPHA,		// Dest blend
					D3D12_BLEND_OP_ADD,				// Blend OP
					D3D12_BLEND_ONE,				// Src blend alpha
					D3D12_BLEND_ONE,				// Dest blend alpha
					D3D12_BLEND_OP_ADD,				// Blend OP alpha
					D3D12_LOGIC_OP_NOOP,			// Logic OP
					D3D12_COLOR_WRITE_ENABLE_ALL,	// Render target write mask
				},
				{},{},{},{},{},{},{}
			}
		};

		const D3D12_BLEND_DESC alpha_blend{
			0,										// Alpha to coverage enable
			0,										// Independent blend enable
			{
				{
					1,								// Blend Enable
					0,								// Logic OP enable
					D3D12_BLEND_SRC_ALPHA,			// Src blend
					D3D12_BLEND_INV_SRC_ALPHA,		// Dest blend
					D3D12_BLEND_OP_ADD,				// Blend OP
					D3D12_BLEND_ONE,				// Src blend alpha
					D3D12_BLEND_ONE,				// Dest blend alpha
					D3D12_BLEND_OP_ADD,				// Blend OP alpha
					D3D12_LOGIC_OP_NOOP,			// Logic OP
					D3D12_COLOR_WRITE_ENABLE_ALL,	// Render target write mask
				},
				{},{},{},{},{},{},{}
			}
		};

		const D3D12_BLEND_DESC additive{
			0,										// Alpha to coverage enable
			0,										// Independent blend enable
			{
				{
					1,								// Blend Enable
					0,								// Logic OP enable
					D3D12_BLEND_ONE,				// Src blend
					D3D12_BLEND_ONE,				// Dest blend
					D3D12_BLEND_OP_ADD,				// Blend OP
					D3D12_BLEND_ONE,				// Src blend alpha
					D3D12_BLEND_ONE,				// Dest blend alpha
					D3D12_BLEND_OP_ADD,				// Blend OP alpha
					D3D12_LOGIC_OP_NOOP,			// Logic OP
					D3D12_COLOR_WRITE_ENABLE_ALL,	// Render target write mask
				},
				{},{},{},{},{},{},{}
			}
		};

		const D3D12_BLEND_DESC premultiplied{
			0,										// Alpha to coverage enable
			0,										// Independent blend enable
			{
				{
					0,								// Blend Enable
					0,								// Logic OP enable
					D3D12_BLEND_ONE,				// Src blend
					D3D12_BLEND_INV_SRC_ALPHA,		// Dest blend
					D3D12_BLEND_OP_ADD,				// Blend OP
					D3D12_BLEND_ONE,				// Src blend alpha
					D3D12_BLEND_ONE,				// Dest blend alpha
					D3D12_BLEND_OP_ADD,				// Blend OP alpha
					D3D12_LOGIC_OP_NOOP,			// Logic OP
					D3D12_COLOR_WRITE_ENABLE_ALL,	// Render target write mask
				},
				{},{},{},{},{},{},{}
			}
		};
	} blend_state;

	constexpr struct {
		const D3D12_STATIC_SAMPLER_DESC static_point {
			D3D12_FILTER_MIN_MAG_MIP_POINT,			// Filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address U
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address V
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address W
			0.f,									// Mip LOD bias
			1,										// Max anisotropy
			D3D12_COMPARISON_FUNC_ALWAYS,			// Comparison function
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,	// Border color
			0.f,									// Min LOD
			D3D12_FLOAT32_MAX,						// Max LOD
			0,										// Shader Register
			0,										// Register Space
			D3D12_SHADER_VISIBILITY_PIXEL			// Shader Visibility
		};

		const D3D12_STATIC_SAMPLER_DESC static_linear {
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,		// Filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address U
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address V
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address W
			0.f,									// Mip LOD bias
			1,										// Max anisotropy
			D3D12_COMPARISON_FUNC_ALWAYS,			// Comparison function
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,	// Border color
			0.f,									// Min LOD
			D3D12_FLOAT32_MAX,						// Max LOD
			0,										// Shader Register
			0,										// Register Space
			D3D12_SHADER_VISIBILITY_PIXEL			// Shader Visibility
		};

		const D3D12_STATIC_SAMPLER_DESC static_anisotropic {
			D3D12_FILTER_ANISOTROPIC,				// Filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address U
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address V
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,		// Address W
			0.f,									// Mip LOD bias
			1,										// Max anisotropy
			D3D12_COMPARISON_FUNC_ALWAYS,			// Comparison function
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,	// Border color
			0.f,									// Min LOD
			D3D12_FLOAT32_MAX,						// Max LOD
			0,										// Shader Register
			0,										// Register Space
			D3D12_SHADER_VISIBILITY_PIXEL			// Shader Visibility
		};
	} sampler_state;

	constexpr D3D12_STATIC_SAMPLER_DESC static_sampler(D3D12_STATIC_SAMPLER_DESC static_sampler, u32 shader_register, u32 register_space, D3D12_SHADER_VISIBILITY visibility) {
		D3D12_STATIC_SAMPLER_DESC sampler{ static_sampler };
		sampler.ShaderRegister = shader_register;
		sampler.RegisterSpace = register_space;
		sampler.ShaderVisibility = visibility;

		return sampler;
	}
 
	constexpr u64 align_size_for_constant_buffer(u64 size) {
		return math::align_size_up<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(size);
	}

	constexpr u64 align_size_for_texture(u64 size) { return math::align_size_up<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(size); }

	class D3D12ResourceBarrier {
		public:
			constexpr static u32 max_resource_barriers{ 64 };
			constexpr void add(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, u32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
				assert(resource);
				assert(_offset < max_resource_barriers);

				D3D12_RESOURCE_BARRIER& barrier{ _barriers[_offset] };
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Flags = flags;
				barrier.Transition.pResource = resource;
				barrier.Transition.Subresource = subresource;
				barrier.Transition.StateBefore = before;
				barrier.Transition.StateAfter = after;

				++_offset;
			}

			constexpr void add(ID3D12Resource* resource, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) {
				assert(resource);
				assert(_offset < max_resource_barriers);

				D3D12_RESOURCE_BARRIER& barrier{ _barriers[_offset] };
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrier.Flags = flags;
				barrier.UAV.pResource = resource;

				++_offset;
			}

			constexpr void add(ID3D12Resource* resource_before, ID3D12Resource* resource_after, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) {
				assert(resource_before && resource_after);
				assert(_offset < max_resource_barriers);

				D3D12_RESOURCE_BARRIER& barrier{ _barriers[_offset] };
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
				barrier.Flags = flags;
				barrier.Aliasing.pResourceBefore = resource_before;
				barrier.Aliasing.pResourceAfter = resource_after;

				++_offset;
			}

			void apply(id3d12_graphics_command_list* cmd_list) {
				assert(_offset);
				cmd_list->ResourceBarrier(_offset, _barriers);
				_offset = 0;
			}

		private:
			D3D12_RESOURCE_BARRIER _barriers[max_resource_barriers]{};
			u32 _offset{ 0 };
	};

	void transition_resource(id3d12_graphics_command_list* cmd_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE, u32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	ID3D12RootSignature* create_root_signature(const D3D12_ROOT_SIGNATURE_DESC1& desc);

	struct D3D12DescriptorRange : public D3D12_DESCRIPTOR_RANGE1 {
		constexpr explicit D3D12DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 descriptor_count, u32 shader_register, u32 space = 0, D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE, u32 offset_from_table_start = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) : D3D12_DESCRIPTOR_RANGE1{ range_type, descriptor_count, shader_register, space, flags, offset_from_table_start } {}
	};

	struct D3D12RootParameter : public D3D12_ROOT_PARAMETER1 {
		constexpr void as_constants(u32 num_constants, D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0) {
			ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			ShaderVisibility = visibility;
			Constants.Num32BitValues = num_constants;
			Constants.ShaderRegister = shader_register;
			Constants.RegisterSpace = space;
		}

		constexpr void as_cbv(D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE) {
			as_descriptor(D3D12_ROOT_PARAMETER_TYPE_CBV, visibility, shader_register, space, flags);
		}

		constexpr void as_srv(D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE) {
			as_descriptor(D3D12_ROOT_PARAMETER_TYPE_SRV, visibility, shader_register, space, flags);
		}

		constexpr void as_uav(D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space = 0, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE) {
			as_descriptor(D3D12_ROOT_PARAMETER_TYPE_UAV, visibility, shader_register, space, flags);
		}

		constexpr void as_descriptor_table(D3D12_SHADER_VISIBILITY visibility, const D3D12DescriptorRange* ranges, u32 range_count) {
			ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			ShaderVisibility = visibility;
			DescriptorTable.NumDescriptorRanges = range_count;
			DescriptorTable.pDescriptorRanges = ranges;
		}

		private:
			constexpr void as_descriptor(D3D12_ROOT_PARAMETER_TYPE type, D3D12_SHADER_VISIBILITY visibility, u32 shader_register, u32 space, D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE) {
				ParameterType = type;
				ShaderVisibility = visibility;
				Descriptor.ShaderRegister = shader_register;
				Descriptor.RegisterSpace = space;
				Descriptor.Flags = flags;
			}
	};

	struct D3D12RootSignatureDesc : public D3D12_ROOT_SIGNATURE_DESC1 {
		constexpr static D3D12_ROOT_SIGNATURE_FLAGS default_flags{
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
			D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
		};

		constexpr explicit D3D12RootSignatureDesc(const D3D12RootParameter* parameters, u32 parameter_count, D3D12_ROOT_SIGNATURE_FLAGS flags = default_flags, const D3D12_STATIC_SAMPLER_DESC* static_sampler = nullptr, u32 sampler_count = 0) : D3D12_ROOT_SIGNATURE_DESC1{ parameter_count, parameters, sampler_count, static_sampler, flags } {}

		ID3D12RootSignature* create() const {
			return create_root_signature(*this);
		}
	};

	#pragma warning(push)
	#pragma warning(disable: 4324)
	template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, typename T> class alignas(void*) D3D12PipelineStateSubobject {
		public:
			D3D12PipelineStateSubobject() = default;
			constexpr explicit D3D12PipelineStateSubobject(T subobject) : _type{ type }, _subobject{ subobject } {}
			D3D12PipelineStateSubobject& operator=(const T& subobject) {
				_subobject = subobject;
				return *this;
			}

		private:
			const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _type{ type };
			T _subobject{};
	};

	#define PSS(name, ...) using d3d12_pipeline_state_subobject_##name = D3D12PipelineStateSubobject<__VA_ARGS__>;
	PSS(root_signature, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*);
	PSS(vs, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE);
	PSS(ps, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE);
	PSS(ds, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS, D3D12_SHADER_BYTECODE);
	PSS(hs, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS, D3D12_SHADER_BYTECODE);
	PSS(gs, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS, D3D12_SHADER_BYTECODE);
	PSS(cs, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE);
	PSS(stream_output, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT, D3D12_STREAM_OUTPUT_DESC);
	PSS(blend, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC);
	PSS(sample_mask, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, u32);
	PSS(rasterizer, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC);
	PSS(depth_stencil, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC);
	PSS(input_layout, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC);
	PSS(ib_strip_cut_value, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE);
	PSS(primitive_topology, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE);
	PSS(render_target_formats, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY);
	PSS(depth_stencil_format, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT);
	PSS(sample_desc, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC);
	PSS(node_mask, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK, u32);
	PSS(cached_pso, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO, D3D12_CACHED_PIPELINE_STATE);
	PSS(flags, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS, D3D12_PIPELINE_STATE_FLAGS);
	PSS(depth_stencil1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1);
	PSS(view_instancing, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC);
	PSS(as, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE);
	PSS(ms, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE);
	#undef PSS

	struct D3D12PipelineStateSubobjectStream {
		d3d12_pipeline_state_subobject_root_signature root_signature{ nullptr };
		d3d12_pipeline_state_subobject_vs vs{};
		d3d12_pipeline_state_subobject_ps ps{};
		d3d12_pipeline_state_subobject_ds ds{};
		d3d12_pipeline_state_subobject_hs hs{};
		d3d12_pipeline_state_subobject_gs gs{};
		d3d12_pipeline_state_subobject_cs cs{};
		d3d12_pipeline_state_subobject_stream_output stream_output{};
		d3d12_pipeline_state_subobject_blend blend{ blend_state.disabled };
		d3d12_pipeline_state_subobject_sample_mask sample_mask{ UINT_MAX };
		d3d12_pipeline_state_subobject_rasterizer rasterizer{ rasterizer_state.no_cull };
		d3d12_pipeline_state_subobject_input_layout input_layout{};
		d3d12_pipeline_state_subobject_ib_strip_cut_value ib_strip_cut_value{};
		d3d12_pipeline_state_subobject_primitive_topology primitive_topology{};
		d3d12_pipeline_state_subobject_render_target_formats render_target_formats{};
		d3d12_pipeline_state_subobject_depth_stencil_format depth_stencil_format{};
		d3d12_pipeline_state_subobject_sample_desc sample_desc{ {1, 0} };
		d3d12_pipeline_state_subobject_node_mask node_mask{};
		d3d12_pipeline_state_subobject_cached_pso cached_pso{};
		d3d12_pipeline_state_subobject_flags flags{};
		d3d12_pipeline_state_subobject_depth_stencil1 depth_stencil1{};
		d3d12_pipeline_state_subobject_view_instancing view_instancing{};
		d3d12_pipeline_state_subobject_as as{};
		d3d12_pipeline_state_subobject_ms ms{};
	};

	ID3D12PipelineState* create_pipeline_state(D3D12_PIPELINE_STATE_STREAM_DESC desc);
	ID3D12PipelineState* create_pipeline_state(void* stream, u64 stream_size);

	ID3D12Resource* create_buffer(const void* data, u32 buffer_size, bool is_cpu_accessible = false, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, ID3D12Heap* heap = nullptr, u64 heap_offset = 0);
}