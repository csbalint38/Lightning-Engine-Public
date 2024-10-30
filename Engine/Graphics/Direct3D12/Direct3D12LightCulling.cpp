#include "Direct3D12LightCulling.h"
#include "Direct3D12Core.h"
#include "Shaders/ShaderTypes.h"
#include "Direct3D12Shaders.h"
#include "Direct3D12Light.h"
#include "Direct3D12Camera.h"
#include "Direct3D12GPass.h"

namespace lightning::graphics::direct3d12::delight {
	namespace {
		struct LightCullingRootParameter {
			enum Parameter : u32 {
				GLOBAL_SHADER_DATA,
				CONSTANTS,
				FRUSTUMS_OUT_OR_INDEX_COUNTER,
				FRUSTUMS_IN,
				CULLING_INFO,
				BOUNDING_SPHERES,
				LIGHT_GRID_OPAQUE,
				LIGHT_INDEX_LIST_OPAQUE,

				count
			};
		};

		struct CullingParameters {
			D3D12Buffer frustums;
			D3D12Buffer light_grid_and_index_list;
			UAVClearebleBuffer light_index_counter;
			hlsl::LightCullingDispatchParameters grid_frustums_dispatch_params{};
			hlsl::LightCullingDispatchParameters light_culling_dispatch_params{};
			u32 frustum_count{ 0 };
			u32 view_width{ 0 };
			u32 view_height{ 0 };
			f32 camera_fov{ 0.f };
			D3D12_GPU_VIRTUAL_ADDRESS light_index_list_opaque_buffer{ 0 };
			bool has_lights{ true };
		};

		struct LightCuller {
			CullingParameters cullers[FRAME_BUFFER_COUNT]{};
		};

		constexpr u32 max_lights_per_tile{ 256 };

		ID3D12RootSignature* light_culling_root_signature{ nullptr };
		ID3D12PipelineState* grid_frustum_pso{ nullptr };
		ID3D12PipelineState* light_culling_pso{ nullptr };
		util::free_list<LightCuller> light_cullers;


		bool create_root_signatures() {
			assert(!light_culling_root_signature);
			using param = LightCullingRootParameter;
			d3dx::D3D12RootParameter parameters[param::count]{};
			parameters[param::GLOBAL_SHADER_DATA].as_cbv(D3D12_SHADER_VISIBILITY_ALL, 0);
			parameters[param::CONSTANTS].as_cbv(D3D12_SHADER_VISIBILITY_ALL, 1);
			parameters[param::FRUSTUMS_OUT_OR_INDEX_COUNTER].as_uav(D3D12_SHADER_VISIBILITY_ALL, 0);
			parameters[param::FRUSTUMS_IN].as_srv(D3D12_SHADER_VISIBILITY_ALL, 0);
			parameters[param::CULLING_INFO].as_srv(D3D12_SHADER_VISIBILITY_ALL, 1);
			parameters[param::BOUNDING_SPHERES].as_srv(D3D12_SHADER_VISIBILITY_ALL, 2);
			parameters[param::LIGHT_GRID_OPAQUE].as_uav(D3D12_SHADER_VISIBILITY_ALL, 1);
			parameters[param::LIGHT_INDEX_LIST_OPAQUE].as_uav(D3D12_SHADER_VISIBILITY_ALL, 3);

			light_culling_root_signature = d3dx::D3D12RootSignatureDesc{ &parameters[0], _countof(parameters) }.create();
			NAME_D3D12_OBJECT(light_culling_root_signature, L"Light Culling Root Signature");

			return light_culling_root_signature != nullptr;
		}

		bool create_psos() {
			{
				assert(!grid_frustum_pso);

				struct {
					d3dx::d3d12_pipeline_state_subobject_root_signature root_signature{ light_culling_root_signature };
					d3dx::d3d12_pipeline_state_subobject_cs cs{ shaders::get_engine_shader(shaders::EngineShader::GRID_FRUSTUMS_CS) };
				} stream;

				grid_frustum_pso = d3dx::create_pipeline_state(&stream, sizeof(stream));
				NAME_D3D12_OBJECT(grid_frustum_pso, L"Grid Frustums PSO");
			}
			{
				assert(!light_culling_pso);

				struct {
					d3dx::d3d12_pipeline_state_subobject_root_signature root_signature{ light_culling_root_signature };
					d3dx::d3d12_pipeline_state_subobject_cs cs{ shaders::get_engine_shader(shaders::EngineShader::LIGHT_CULLING_CS) };
				} stream;

				light_culling_pso = d3dx::create_pipeline_state(&stream, sizeof(stream));
				NAME_D3D12_OBJECT(light_culling_pso, L"Light Culling PSO");
			}

			return grid_frustum_pso != nullptr && light_culling_pso != nullptr;
		}

		void resize_buffers(CullingParameters& culler) {
			const u32 frustum_count{ culler.frustum_count };
			const u32 frustums_buffer_size{ sizeof(hlsl::Frustum) * frustum_count };
			const u32 light_grid_buffer_size{ (u32)math::align_size_up<sizeof(math::v4)>(sizeof(math::u32v2) * frustum_count) };
			const u32 light_index_list_buffer_size{ (u32)math::align_size_up<sizeof(math::v4)>(sizeof(u32) * max_lights_per_tile * frustum_count) };
			const u32 light_grid_and_index_list_buffer_size{ light_grid_buffer_size + light_index_list_buffer_size };

			D3D12BufferInitInfo info{};
			info.alignment = sizeof(math::v4);
			info.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			if (frustums_buffer_size > culler.frustums.size()) {
				info.size = frustums_buffer_size;
				culler.frustums = D3D12Buffer{ info, false };
				NAME_D3D12_OBJECT_INDEXED(culler.frustums.buffer(), frustum_count, L"Light Grid Frustums Buffer - count");
			}

			if (light_grid_and_index_list_buffer_size > culler.light_grid_and_index_list.size()) {
				info.size = light_grid_and_index_list_buffer_size;
				culler.light_grid_and_index_list = D3D12Buffer{ info, false };

				const D3D12_GPU_VIRTUAL_ADDRESS light_grid_opaque_buffer{ culler.light_grid_and_index_list.gpu_address() };
				culler.light_index_list_opaque_buffer = light_grid_opaque_buffer + light_grid_buffer_size;
				NAME_D3D12_OBJECT_INDEXED(culler.light_grid_and_index_list.buffer(), light_grid_and_index_list_buffer_size, L"Light Grid and Index List Buffer - size");

				if (!culler.light_index_counter.buffer()) {
					info = UAVClearebleBuffer::get_default_init_info(1);
					culler.light_index_counter = UAVClearebleBuffer{ info };
					NAME_D3D12_OBJECT_INDEXED(culler.light_index_counter.buffer(), core::current_frame_index(), L"Light Index Counter Buffer ");
				}
			}
		}

		void resize(CullingParameters& culler) {
			constexpr u32 tile_size{ light_culling_tile_size };
			assert(culler.view_width >= tile_size && culler.view_height >= tile_size);
			const math::u32v2 tile_count{
				(u32)math::align_size_up<tile_size>(culler.view_width) / tile_size,
				(u32)math::align_size_up<tile_size>(culler.view_height) / tile_size,
			};

			culler.frustum_count = tile_count.x * tile_count.y;

			{
				hlsl::LightCullingDispatchParameters& params{ culler.grid_frustums_dispatch_params };
				params.num_threds = tile_count;
				params.num_thread_groups.x = (u32)math::align_size_up<tile_size>(tile_count.x) / tile_size;
				params.num_thread_groups.y = (u32)math::align_size_up<tile_size>(tile_count.y) / tile_size;
			}
			{
				hlsl::LightCullingDispatchParameters& params{ culler.light_culling_dispatch_params };
				params.num_threds.x = tile_count.x * tile_size;
				params.num_threds.y = tile_count.y * tile_size;
				params.num_thread_groups = tile_count;
			}

			resize_buffers(culler);
		}

		void calculate_grid_frustums(const CullingParameters& culler, id3d12_graphics_command_list* const cmd_list, const D3D12FrameInfo& info, d3dx::D3D12ResourceBarrier& barriers) {
			ConstantBuffer& cbuffer{ core::c_buffer() };
			hlsl::LightCullingDispatchParameters* const buffer{ cbuffer.allocate<hlsl::LightCullingDispatchParameters>() };
			const hlsl::LightCullingDispatchParameters& params{ culler.grid_frustums_dispatch_params };
			memcpy(buffer, &params, sizeof(hlsl::LightCullingDispatchParameters));

			// TEMP
			barriers.add(culler.frustums.buffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			barriers.apply(cmd_list);

			using param = LightCullingRootParameter;
			cmd_list->SetComputeRootSignature(light_culling_root_signature);
			cmd_list->SetPipelineState(grid_frustum_pso);
			cmd_list->SetComputeRootConstantBufferView(param::GLOBAL_SHADER_DATA, info.global_shader_data);
			cmd_list->SetComputeRootConstantBufferView(param::CONSTANTS, cbuffer.gpu_address(buffer));
			cmd_list->SetComputeRootUnorderedAccessView(param::FRUSTUMS_OUT_OR_INDEX_COUNTER, culler.frustums.gpu_address());
			cmd_list->Dispatch(params.num_thread_groups.x, params.num_thread_groups.y, 1);

			// TEMP
			barriers.add(culler.frustums.buffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		void _declspec(noinline) resize_and_calculate_grid_frustums(CullingParameters& culler, id3d12_graphics_command_list* const cmd_list, const D3D12FrameInfo& info, d3dx::D3D12ResourceBarrier& barriers) {
			culler.camera_fov = info.camera->field_of_view();
			culler.view_width = info.surface_width;
			culler.view_height = info.surface_height;

			resize(culler);
			calculate_grid_frustums(culler, cmd_list, info, barriers);
		}
	}

	bool initialize() {
		return create_root_signatures() && create_psos() && light::initialize();
	}

	void shutdown() {
		light::shutdown();
		assert(light_culling_root_signature && grid_frustum_pso && light_culling_pso);
		core::deferred_release(light_culling_root_signature);
		core::deferred_release(grid_frustum_pso);
		core::deferred_release(light_culling_pso);
	}

	[[nodiscard]] id::id_type add_culler() { return light_cullers.add(); }

	void remove_culler(id::id_type id) {
		assert(id::is_valid(id));
		light_cullers.remove(id);
	}

	void cull_lights(id3d12_graphics_command_list* const cmd_list, const D3D12FrameInfo& info, d3dx::D3D12ResourceBarrier& barriers) {
		const id::id_type id{ info.light_culling_id };
		assert(id::is_valid(id));
		CullingParameters& culler{ light_cullers[id].cullers[info.frame_index] };

		if (info.surface_width != culler.view_width || info.surface_height != culler.view_height || !math::is_equal(info.camera->field_of_view(), culler.camera_fov)) {
			resize_and_calculate_grid_frustums(culler, cmd_list, info, barriers);
		}

		hlsl::LightCullingDispatchParameters& params{ culler.light_culling_dispatch_params };
		params.num_lights = light::cullable_light_count(info.info->light_set_key);
		params.depth_buffer_srv_index = gpass::depth_buffer().srv().index;

		if (!params.num_lights && !culler.has_lights) return;

		culler.has_lights = params.num_lights > 0;

		ConstantBuffer& cbuffer{ core::c_buffer() };
		hlsl::LightCullingDispatchParameters* const buffer{ cbuffer.allocate<hlsl::LightCullingDispatchParameters>() };
		memcpy(buffer, &params, sizeof(hlsl::LightCullingDispatchParameters));

		barriers.add(culler.light_grid_and_index_list.buffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		barriers.apply(cmd_list);

		const math::u32v4 clear_value{ 0, 0, 0, 0 };
		culler.light_index_counter.clear_uav(cmd_list, &clear_value.x);

		cmd_list->SetComputeRootSignature(light_culling_root_signature);
		cmd_list->SetPipelineState(light_culling_pso);
		using param = LightCullingRootParameter;
		cmd_list->SetComputeRootConstantBufferView(param::GLOBAL_SHADER_DATA, info.global_shader_data);
		cmd_list->SetComputeRootConstantBufferView(param::CONSTANTS, cbuffer.gpu_address(buffer));
		cmd_list->SetComputeRootUnorderedAccessView(param::FRUSTUMS_OUT_OR_INDEX_COUNTER, culler.light_index_counter.gpu_address());
		cmd_list->SetComputeRootShaderResourceView(param::FRUSTUMS_IN, culler.frustums.gpu_address());
		cmd_list->SetComputeRootShaderResourceView(param::CULLING_INFO, light::culling_info_buffer(info.frame_index));
		cmd_list->SetComputeRootShaderResourceView(param::BOUNDING_SPHERES, light::bounding_spheres_buffer(info.frame_index));
		cmd_list->SetComputeRootUnorderedAccessView(param::LIGHT_GRID_OPAQUE, culler.light_grid_and_index_list.gpu_address());
		cmd_list->SetComputeRootUnorderedAccessView(param::LIGHT_INDEX_LIST_OPAQUE, culler.light_index_list_opaque_buffer);

		cmd_list->Dispatch(params.num_thread_groups.x, params.num_thread_groups.y, 1);

		barriers.add(culler.light_grid_and_index_list.buffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// TEMP

	D3D12_GPU_VIRTUAL_ADDRESS frustums(id::id_type light_culling_id, u32 frame_index) {
		assert(frame_index < FRAME_BUFFER_COUNT && id::is_valid(light_culling_id));
		return light_cullers[light_culling_id].cullers[frame_index].frustums.gpu_address();
	}

	D3D12_GPU_VIRTUAL_ADDRESS light_grid_opaque(id::id_type light_culling_id, u32 frame_index) {
		assert(frame_index < FRAME_BUFFER_COUNT && id::is_valid(light_culling_id));
		return light_cullers[light_culling_id].cullers[frame_index].light_grid_and_index_list.gpu_address();
	}
	D3D12_GPU_VIRTUAL_ADDRESS light_index_list_opaque(id::id_type light_culling_id, u32 frame_index) {
		assert(frame_index < FRAME_BUFFER_COUNT && id::is_valid(light_culling_id));
		return light_cullers[light_culling_id].cullers[frame_index].light_index_list_opaque_buffer;
	}

}