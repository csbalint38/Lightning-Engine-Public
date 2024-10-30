#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12::content {

	bool initialize();
	void shutdown();

	namespace submesh {

		struct ViewsCache {
			D3D12_GPU_VIRTUAL_ADDRESS* const position_buffers;
			D3D12_GPU_VIRTUAL_ADDRESS* const element_buffers;
			D3D12_INDEX_BUFFER_VIEW* const index_buffer_views;
			D3D_PRIMITIVE_TOPOLOGY* const primitive_topologies;
			u32* const elements_types;
		};

		id::id_type add(const u8*& data);
		void remove(id::id_type id);
		void get_views(const id::id_type* const gpu_ids, u32 id_count, const ViewsCache& cache);
	}

	namespace texture {
		id::id_type add(const u8* const data);
		void remove(id::id_type id);
		void get_descriptor_indicies(const id::id_type* const texture_ids, u32 id_count, u32* const indicies);
	}

	namespace material {

		struct MaterialsCache {
			ID3D12RootSignature** const root_signatures;
			MaterialType::Type* const material_types;
			u32** const descriptor_indices;
			u32* const texture_count;
			MaterialSurface** const material_surfaces;
		};

		id::id_type add(MaterialInitInfo info);
		void remove(id::id_type id);
		void get_materials(const id::id_type* const material_ids, u32 material_count, const MaterialsCache& cache, u32& descriptor_index_count);
	}

	namespace render_item {

		struct ItemsCache {
			id::id_type* const entity_ids;
			id::id_type* const submesh_gpu_ids;
			id::id_type* const material_ids;
			ID3D12PipelineState** const gpass_psos;
			ID3D12PipelineState** const depth_psos;
		};

		id::id_type add(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count, const id::id_type* const material_ids);
		void remove(id::id_type id);
		void get_d3d12_render_items_id(const FrameInfo& info, util::vector<id::id_type>& d3d12_render_item_ids);
		void get_items(const id::id_type* const d3d12_render_item_ids, u32 id_count, const ItemsCache& cache);
	}
}