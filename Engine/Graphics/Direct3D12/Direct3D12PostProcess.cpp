#include "Direct3D12PostProcess.h"
#include "Direct3D12Core.h"
#include "Direct3D12Shaders.h"
#include "Direct3D12Surface.h"
#include "Direct3D12GPass.h"
#include "Direct3D12LightCulling.h"

namespace lightning::graphics::direct3d12::fx {
	namespace {

		struct FXRootParamIndicies {
			enum : u32 {
				GLOBAL_SHADER_DATA,
				ROOT_CONSTANTS,

				// TEMP
				FRUSTUMS,
				LIGHT_GRID_OPAQUE,

				count
			};
		};

		ID3D12RootSignature* fx_root_sig{ nullptr };
		ID3D12PipelineState* fx_pso{ nullptr };

		bool create_fx_pso_and_root_signature() {
			assert(!fx_root_sig && !fx_pso);

			using idx = FXRootParamIndicies;
			d3dx::D3D12RootParameter parameters[idx::count]{};
			parameters[idx::GLOBAL_SHADER_DATA].as_cbv(D3D12_SHADER_VISIBILITY_PIXEL, 0);
			parameters[idx::ROOT_CONSTANTS].as_constants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
			parameters[idx::FRUSTUMS].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 0);
			parameters[idx::LIGHT_GRID_OPAQUE].as_srv(D3D12_SHADER_VISIBILITY_PIXEL, 1);

			d3dx::D3D12RootSignatureDesc root_signature{ &parameters[0], _countof(parameters) };
			root_signature.Flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			fx_root_sig = root_signature.create();
			assert(fx_root_sig);
			NAME_D3D12_OBJECT(fx_root_sig, L"Post-process FX Root Signature");

			struct {
				d3dx::d3d12_pipeline_state_subobject_root_signature root_signature{ fx_root_sig };
				d3dx::d3d12_pipeline_state_subobject_vs vs{ shaders::get_engine_shader(shaders::EngineShader::FULLSCREEN_TRIANGLE_VS) };
				d3dx::d3d12_pipeline_state_subobject_ps ps{ shaders::get_engine_shader(shaders::EngineShader::POST_PROCESS_PS) };
				d3dx::d3d12_pipeline_state_subobject_primitive_topology primitive_topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
				d3dx::d3d12_pipeline_state_subobject_render_target_formats render_target_formats;
				d3dx::d3d12_pipeline_state_subobject_rasterizer rasterizer{ d3dx::rasterizer_state.no_cull };
			} stream;

			D3D12_RT_FORMAT_ARRAY  rtf_array{};
			rtf_array.NumRenderTargets = 1;
			rtf_array.RTFormats[0] = D3D12Surface::default_back_buffer_format;

			stream.render_target_formats = rtf_array;

			fx_pso = d3dx::create_pipeline_state(&stream, sizeof(stream));
			NAME_D3D12_OBJECT(fx_pso, L"Post-process FX Pipeline State Object");

			return fx_root_sig && fx_pso;
		}
	}

	bool initialize() {
		return create_fx_pso_and_root_signature();
	}

	void shutdown() {
		core::release(fx_root_sig);
		core::release(fx_pso);
	}

	void post_process(id3d12_graphics_command_list* cmd_list, const D3D12FrameInfo& info, D3D12_CPU_DESCRIPTOR_HANDLE target_rtv) {
		const u32 frame_index{ info.frame_index };
		const id::id_type light_culling_id{ info.light_culling_id };

		cmd_list->SetGraphicsRootSignature(fx_root_sig);
		cmd_list->SetPipelineState(fx_pso);

		using idx = FXRootParamIndicies;
		cmd_list->SetGraphicsRootConstantBufferView(idx::GLOBAL_SHADER_DATA, info.global_shader_data);
		cmd_list->SetGraphicsRoot32BitConstant(idx::ROOT_CONSTANTS, gpass::main_buffer().srv().index, 0);
		cmd_list->SetGraphicsRootShaderResourceView(idx::FRUSTUMS, delight::frustums(light_culling_id, frame_index));
		cmd_list->SetGraphicsRootShaderResourceView(idx::LIGHT_GRID_OPAQUE, delight::light_grid_opaque(light_culling_id, frame_index));

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		cmd_list->OMSetRenderTargets(1, &target_rtv, 1, nullptr);
		cmd_list->DrawInstanced(3, 1, 0, 0);
	}
}