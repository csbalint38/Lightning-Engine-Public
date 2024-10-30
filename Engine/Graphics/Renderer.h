#pragma once

#include "CommonHeaders.h"
#include "Platform/Window.h"
#include "EngineAPI/Camera.h"
#include "EngineAPI/Light.h"

#ifdef OPAQUE
#undef OPAQUE
#endif

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif

namespace lightning::graphics {

	struct FrameInfo {
		id::id_type* render_item_ids{ nullptr };
		f32* thresholds{ nullptr };
		u64 light_set_key{ 0 };
		f32 last_frame_time{ 16.7f };
		f32 average_frame_time{ 16.7f };
		u32 render_item_count{ 0 };
		camera_id camera_id{ id::invalid_id };
	};

	DEFINE_TYPED_ID(surface_id);
	
	class Surface {
		private:
			surface_id _id{ id::invalid_id };

		public:
			constexpr explicit Surface(surface_id id) : _id{ id } {}
			constexpr Surface() = default;
			constexpr surface_id get_id() const { return _id; }
			constexpr bool is_valid() const { return id::is_valid(_id); }

			void resize(u32 width, u32 height) const;
			u32 width() const;
			u32 height() const;
			void render(FrameInfo info) const;
	};

	struct RenderSurface {
		platform::Window window{};
		Surface surface{};
	};

	struct DirectionalLightParams {};

	struct PointLightParams {
		math::v3 attenuation;
		f32 range;
	};

	struct  SpotLightParams {
		math::v3 attenuation;
		f32 range;
		f32 umbra;
		f32 penumbra;
	};

	struct LightInitInfo {
		u64 light_set_key{ 0 };
		id::id_type entity_id{ id::invalid_id };
		Light::Type type{};
		f32 intensity{ 1.f };
		math::v3 color{ 1.f, 1.f, 1.f };
		union {
			DirectionalLightParams directional_params;
			PointLightParams point_params;
			SpotLightParams spot_params;
		};
		bool is_enabled{ true };
	};

	struct LightParameter {
		enum Parameter : u32 {
			IS_ENABLED,
			INTENSITY,
			COLOR,
			ATTENUATION,
			RANGE,
			UMBRA,
			PENUMBRA,
			TYPE,
			ENTITY_ID,

			count
		};
	};

	struct CameraParameter {
		enum Parameter : u32 {
			UP_VECTOR,
			FIELD_OF_VIEW,
			ASPECT_RATIO,
			VIEW_WIDTH,
			VIEW_HEIGHT,
			NEAR_Z,
			FAR_Z,
			VIEW,
			PROJECTION,
			INVERSE_PROJECTION,
			VIEW_PROJECTION,
			INVERSE_VIEW_PROJECTION,
			TYPE,
			ENTITY_ID,

			count
		};
	};

	struct CameraInitInfo {
		id::id_type entity_id{ id::invalid_id };
		Camera::Type type{};
		math::v3 up;
		union {
			f32 field_of_view;
			f32 view_width;
		};
		union {
			f32 aspect_ratio;
			f32 view_height;
		};
		f32 near_z;
		f32 far_z;
	};

	struct PerspectiveCameraInitInfo : public CameraInitInfo {
		explicit PerspectiveCameraInitInfo(id::id_type id) {
			assert(id::is_valid(id));
			entity_id = id;
			type = Camera::PERSPECTIVE;
			up = { 0.f, 1.f, 0.f };
			field_of_view = .25f;
			aspect_ratio = 16.f / 10.f;
			near_z = 0.01f;
			far_z = 1000.f;
		}
	};

	struct OrtographicCameraInitInfo : public CameraInitInfo {
		explicit OrtographicCameraInitInfo(id::id_type id) {
			assert(id::is_valid(id));
			entity_id = id;
			type = Camera::ORTOGRAPHIC;
			up = { 0.f, 1.f, 0.f };
			view_width = 1920.f;
			view_height = 1080;
			near_z = 0.01f;
			far_z = 1000.f;
		}
	};

	struct ShaderFlags {
		enum Flags : u32 {
			NONE = 0x0,
			VERTEX = 0x01,
			HULL = 0x02,
			DOMAIN = 0x04,
			GEOMETRY = 0x08,
			PIXEL = 0x10,
			COMPUTE = 0x20,
			AMPLIFICATION = 0x40,
			MESH = 0x80
		};
	};

	struct ShaderType {
		enum Type : u32 {
			VERTEX = 0,
			HULL,
			DOMAIN,
			GEOMETRY,
			PIXEL,
			COMPUTE,
			AMPLIFICATION,
			MESH,

			count
		};
	};

	struct MaterialType {
		enum Type : u32 {
			OPAQUE,
			TRANSPARENT,
			UNLIT,
			CLEAR_COAT,
			CLOTH,
			SKIN,
			FOLIAGE,
			HAIR,
			
			count
		};
	};

	struct MaterialSurface {
		math::v4 base_color{ 1.f, 1.f, 1.f, 1.f };
		math::v3 emissive{ 0.f, 0.f, 0.f };
		f32 emissive_intensity{ 1.f };
		f32 ambient_occlusion{ 1.f };
		f32 metallic { 0.f };
		f32 roughness { 1.f };
	};

	struct MaterialInitInfo {
		id::id_type* texture_ids;
		MaterialSurface surface;
		MaterialType::Type type;
		u32 texture_count;
		id::id_type shader_ids[ShaderType::Type::count]{ id::invalid_id, id::invalid_id, id::invalid_id, id::invalid_id, id::invalid_id, id::invalid_id, id::invalid_id, id::invalid_id };
	};

	struct PrimitiveTopology {
		enum Type : u32 {
			POINT_LIST = 1,
			LINE_LIST,
			LINE_STRIP,
			TRIANGLE_LIST,
			TRIANGLE_STRIP,

			count
		};
	};

	// Use this if the Engine supports multiple graphics renderers
	enum class GraphicsPlatform : u32 {
		DIRECT3D12 = 0,
		VULKAN = 1,
		OPEN_GL = 2,
	};

	bool initialize(GraphicsPlatform platform);
	void shutdown();

	const char* get_engine_shaders_path();
	const char* get_engine_shaders_path(graphics::GraphicsPlatform platform);

	Surface create_surface(platform::Window window);
	void remove_surface(surface_id id);

	Camera create_camera(CameraInitInfo info);
	void remove_camera(camera_id id);

	id::id_type add_submesh(const u8*& data);
	void remove_submesh(id::id_type id);

	id::id_type add_texture(const u8* const data);
	void remove_texture(id::id_type id);

	void create_light_set(u64 light_set_key);
	void remove_light_set(u64 light_set_key);

	Light create_light(LightInitInfo info);
	void remove_light(light_id id, u64 light_set_key);

	id::id_type add_material(MaterialInitInfo info);
	void remove_material(id::id_type id);

	id::id_type add_render_item(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count, const id::id_type* const material_ids);
	void remove_render_item(id::id_type id);
}