#pragma once
#include "ToolsCommon.h"
#include <fbxsdk.h>

namespace lightning::tools {

	struct SceneData;
	struct Scene;
	struct Mesh;
	struct GeometryImportSettings;

	class FbxContext {
		public:
			FbxContext(const char* file, Scene* scene, SceneData* data, Progression* const progression) : _scene{ scene }, _scene_data{ data }, _progression{ progression } {
				assert(file && _scene && _scene_data && _progression);
				if (initialize_fbx()) {
					load_fbx_file(file);
					assert(is_valid());
				}
			}

			~FbxContext() {
				_fbx_scene->Destroy();
				_fbx_manager->Destroy();
				ZeroMemory(this, sizeof(FbxContext));
			}

			void get_scene(FbxNode* root = nullptr);

			constexpr bool is_valid() const { return _fbx_manager && _fbx_scene; }
			constexpr f32 scene_scale() const { return _scene_scale; }
			constexpr Progression* get_progression() const { return _progression; }

		private:

			bool initialize_fbx();
			void load_fbx_file(const char* file);
			void get_meshes(FbxNode* node, util::vector<Mesh>& meshes, u32 lod_id, f32 lod_threshold);
			void get_mesh(FbxNodeAttribute* attribute, util::vector<Mesh>& meshes, u32 lod_id, f32 lod_treshold);
			void get_lod_group(FbxNodeAttribute* attribute);
			bool get_mesh_data(FbxMesh* fbx_mesh, Mesh& m);

			Scene* _scene{ nullptr };
			SceneData* _scene_data{ nullptr };
			FbxManager* _fbx_manager{ nullptr };
			FbxScene* _fbx_scene{ nullptr };
			Progression* _progression{ nullptr };
			f32 _scene_scale{ 1.f };
	};

}