#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "Direct3D12/Direct3D12Interface.h"

namespace lightning::graphics {
	namespace {
		constexpr const char* engine_shader_paths[]{
			"./shaders/d3d12/shaders.bin"
		};

		PlatformInterface gfx{};

		// use this if Engine supports multiple graphics renderers
		bool set_platform_interface(GraphicsPlatform platform, PlatformInterface& pi) {
			switch (platform) {
			case lightning::graphics::GraphicsPlatform::DIRECT3D12:
				direct3d12::get_platform_interface(pi);
				break;
			default:
				return false;
			}
			assert(gfx.platform == platform);
			return true;
		}
	}

	bool initialize(GraphicsPlatform platform) { return set_platform_interface(platform, gfx) && gfx.initialize(); }
	void shutdown() { if (gfx.platform != (GraphicsPlatform)-1) gfx.shutdown(); }
	Surface create_surface(platform::Window window) { return gfx.surface.create(window); }

	void remove_surface(surface_id id) {
		assert(id::is_valid(id));
		gfx.surface.remove(id);
	}

	const char* get_engine_shaders_path() { return engine_shader_paths[(u32)gfx.platform]; }
	const char* get_engine_shaders_path(GraphicsPlatform platform) { return engine_shader_paths[(u32)platform]; }

	void Surface::resize(u32 width, u32 height) const {
		assert(is_valid());
		gfx.surface.resize(_id, width, height);
	}

	u32 Surface::width() const {
		assert(is_valid());
		return gfx.surface.width(_id);
	}

	u32 Surface::height() const {
		assert(is_valid());
		return gfx.surface.height(_id);
	}

	void Surface::render(FrameInfo info) const {
		assert(is_valid());
		gfx.surface.render(_id, info);
	}

	void create_light_set(u64 light_set_key) {
		gfx.light.create_light_set(light_set_key);
	}

	void remove_light_set(u64 light_set_key) {
		gfx.light.remove_light_set(light_set_key);
	}

	Light create_light(LightInitInfo info) {
		return gfx.light.create(info);
	}

	void remove_light(light_id id, u64 light_set_key) {
		gfx.light.remove(id, light_set_key);
	}

	void Light::is_enabled(bool is_enabled) const {
		assert(is_valid());
		gfx.light.set_parameter(_id, _light_set_key, LightParameter::IS_ENABLED, &is_enabled, sizeof(is_enabled));
	}

	void Light::intensity(f32 intensity) const {
		assert(is_valid());
		gfx.light.set_parameter(_id, _light_set_key, LightParameter::INTENSITY, &intensity, sizeof(intensity));
	}

	void Light::color(math::v3 color) const {
		assert(is_valid());
		gfx.light.set_parameter(_id, _light_set_key, LightParameter::COLOR, &color, sizeof(color));
	}

	void Light::attenuation(math::v3 attenuation) const {
		assert(is_valid());
		gfx.light.set_parameter(_id, _light_set_key, LightParameter::ATTENUATION, &attenuation, sizeof(attenuation));
	}

	void Light::range(f32 range) const {
		assert(is_valid());
		gfx.light.set_parameter(_id, _light_set_key, LightParameter::RANGE, &range, sizeof(range));
	}

	void Light::cone_angles(f32 umbra, f32 penumbra) const {
		assert(is_valid());
		gfx.light.set_parameter(_id, _light_set_key, LightParameter::UMBRA, &umbra, sizeof(umbra));
		gfx.light.set_parameter(_id, _light_set_key, LightParameter::PENUMBRA, &penumbra, sizeof(penumbra));
	}

	bool Light::is_enabled() const {
		assert(is_valid());
		bool is_enabled;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::IS_ENABLED, &is_enabled, sizeof(is_enabled));
		return is_enabled;
	}

	f32 Light::intensity() const {
		assert(is_valid());
		f32 intensity;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::INTENSITY, &intensity, sizeof(intensity));
		return intensity;
	}

	math::v3 Light::color() const {
		assert(is_valid());
		math::v3 color;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::COLOR, &color, sizeof(color));
		return color;
	}

	math::v3 Light::attenuation() const {
		assert(is_valid());
		math::v3 attenuation;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::ATTENUATION, &attenuation, sizeof(attenuation));
		return attenuation;
	}

	f32 Light::range() const {
		assert(is_valid());
		f32 range;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::RANGE, &range, sizeof(range));
		return range;
	}

	f32 Light::umbra() const {
		assert(is_valid());
		f32 umbra;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::UMBRA, &umbra, sizeof(umbra));
		return umbra;
	}

	f32 Light::penumbra() const {
		assert(is_valid());
		f32 penumbra;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::PENUMBRA, &penumbra, sizeof(penumbra));
		return penumbra;
	}

	Light::Type Light::light_type() const {
		assert(is_valid());
		Type type;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::TYPE, &type, sizeof(type));
		return type;
	}

	id::id_type Light::entity_id() const {
		assert(is_valid());
		id::id_type entity_id;
		gfx.light.get_parameter(_id, _light_set_key, LightParameter::ENTITY_ID, &entity_id, sizeof(entity_id));
		return entity_id;
	}

	Camera create_camera(CameraInitInfo info) {
		return gfx.camera.create(info);
	}

	void remove_camera(camera_id id) {
		gfx.camera.remove(id);
	}

	void Camera::up(math::v3 up) const {
		assert(is_valid());
		gfx.camera.set_parameter(_id, CameraParameter::UP_VECTOR, &up, sizeof(up));
	}

	void Camera::field_of_view(f32 fov) const {
		assert(is_valid());
		gfx.camera.set_parameter(_id, CameraParameter::FIELD_OF_VIEW, &fov, sizeof(fov));
	}

	void Camera::aspect_ratio(f32 aspect_ratio) const {
		assert(is_valid());
		gfx.camera.set_parameter(_id, CameraParameter::ASPECT_RATIO, &aspect_ratio, sizeof(aspect_ratio));
	}

	void Camera::view_width(f32 width) const {
		assert(is_valid());
		gfx.camera.set_parameter(_id, CameraParameter::VIEW_WIDTH, &width, sizeof(width));
	}

	void Camera::view_height(f32 height) const {
		assert(is_valid());
		gfx.camera.set_parameter(_id, CameraParameter::VIEW_HEIGHT, &height, sizeof(height));
	}

	void Camera::range(f32 near_z, f32 far_z) const {
		assert(is_valid());
		gfx.camera.set_parameter(_id, CameraParameter::NEAR_Z, &near_z, sizeof(near_z));
		gfx.camera.set_parameter(_id, CameraParameter::FAR_Z, &far_z, sizeof(far_z));
	}

	math::m4x4 Camera::view() const {
		assert(is_valid());
		math::m4x4 matrix;
		gfx.camera.get_parameter(_id, CameraParameter::VIEW, &matrix, sizeof(matrix));
		return matrix;
	}

	math::m4x4 Camera::projection() const {
		assert(is_valid());
		math::m4x4 matrix;
		gfx.camera.get_parameter(_id, CameraParameter::PROJECTION, &matrix, sizeof(matrix));
		return matrix;

	}

	math::m4x4 Camera::inverse_projection() const {
		assert(is_valid());
		math::m4x4 matrix;
		gfx.camera.get_parameter(_id, CameraParameter::INVERSE_PROJECTION, &matrix, sizeof(matrix));
		return matrix;
	}

	math::m4x4 Camera::view_projection() const {
		assert(is_valid());
		math::m4x4 matrix;
		gfx.camera.get_parameter(_id, CameraParameter::VIEW_PROJECTION, &matrix, sizeof(matrix));
		return matrix;
	}

	math::m4x4 Camera::inverse_view_projection() const {
		assert(is_valid());
		math::m4x4 matrix;
		gfx.camera.get_parameter(_id, CameraParameter::INVERSE_VIEW_PROJECTION, &matrix, sizeof(matrix));
		return matrix;
	}

	math::v3 Camera::up() const {
		assert(is_valid());
		math::v3 up_vector;
		gfx.camera.get_parameter(_id, CameraParameter::UP_VECTOR, &up_vector, sizeof(up_vector));
		return up_vector;
	}
	f32 Camera::near_z() const {
		assert(is_valid());
		f32 near_z;
		gfx.camera.get_parameter(_id, CameraParameter::NEAR_Z, &near_z, sizeof(near_z));
		return near_z;
	}

	f32 Camera::far_z() const {
		assert(is_valid());
		f32 far_z;
		gfx.camera.get_parameter(_id, CameraParameter::NEAR_Z, &far_z, sizeof(far_z));
		return far_z;
	}

	f32 Camera::field_of_view() const {
		assert(is_valid());
		f32 field_of_view;
		gfx.camera.get_parameter(_id, CameraParameter::FAR_Z, &field_of_view, sizeof(field_of_view));
		return field_of_view;
	}

	f32 Camera::aspect_ratio() const {
		assert(is_valid());
		f32 aspect_ratio;
		gfx.camera.get_parameter(_id, CameraParameter::ASPECT_RATIO, &aspect_ratio, sizeof(aspect_ratio));
		return aspect_ratio;
	}

	f32 Camera::view_width() const {
		assert(is_valid());
		f32 view_width;
		gfx.camera.get_parameter(_id, CameraParameter::VIEW_WIDTH, &view_width, sizeof(view_width));
		return view_width;
	}

	f32 Camera::view_height() const {
		assert(is_valid());
		f32 view_height;
		gfx.camera.get_parameter(_id, CameraParameter::VIEW_HEIGHT, &view_height, sizeof(view_height));
		return view_height;
	}

	Camera::Type Camera::projection_type() const {
		assert(is_valid());
		Camera::Type projection_type;
		gfx.camera.get_parameter(_id, CameraParameter::TYPE, &projection_type, sizeof(projection_type));
		return projection_type;
	}

	id::id_type Camera::entity_id() const {
		assert(is_valid());
		id::id_type id;
		gfx.camera.get_parameter(_id, CameraParameter::ENTITY_ID, &id, sizeof(id));
		return id;
	}

	id::id_type add_submesh(const u8*& data) {
		return gfx.resources.add_submesh(data);
	}

	void remove_submesh(id::id_type id) {
		gfx.resources.remove_submesh(id);
	}

	id::id_type add_texture(const u8* const data) {
		return gfx.resources.add_texture(data);
	}

	void remove_texture(id::id_type id) {
		gfx.resources.remove_texture(id);
	}

	id::id_type add_material(MaterialInitInfo info) {
		return gfx.resources.add_material(info);
	}

	void remove_material(id::id_type id) {
		gfx.resources.remove_material(id);
	}

	id::id_type add_render_item(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count, const id::id_type* const material_ids) {
		return gfx.resources.add_render_item(entity_id, geometry_content_id, material_count, material_ids);
	}

	void remove_render_item(id::id_type id) {
		gfx.resources.remove_render_item(id);
	}
}