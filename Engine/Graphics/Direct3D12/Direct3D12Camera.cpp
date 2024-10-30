#include "Direct3D12Camera.h"
#include "EngineAPI/GameEntity.h"

namespace lightning::graphics::direct3d12::camera {
	namespace {
		util::free_list<D3D12Camera> cameras;

		void set_up_vector(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			math::v3 up_vector{ *(math::v3*)data };
			assert(sizeof(up_vector) == size);
			camera.up(up_vector);
		}

		constexpr void set_field_of_view(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::PERSPECTIVE);
			f32 fov{ *(f32*)data };
			assert(sizeof(fov) == size);
			camera.field_of_view(fov);
		}

		constexpr void set_aspect_ratio(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::PERSPECTIVE);
			f32 aspect_ratio{ *(f32*)data };
			assert(sizeof(aspect_ratio) == size);
			camera.aspect_ratio(aspect_ratio);
		}

		constexpr void set_view_width(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::ORTOGRAPHIC);
			f32 width{ *(f32*)data };
			assert(sizeof(width) == size);
			camera.view_width(width);
		}

		constexpr void set_view_height(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::ORTOGRAPHIC);
			f32 height{ *(f32*)data };
			assert(sizeof(height) == size);
			camera.view_height(height);
		}

		constexpr void set_near_z(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			f32 near_z{ *(f32*)data };
			assert(sizeof(near_z) == size);
			camera.near_z(near_z);
		}

		constexpr void set_far_z(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size) {
			f32 far_z{ *(f32*)data };
			assert(sizeof(far_z) == size);
			camera.far_z(far_z);
		}

		void get_view(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.view());
		}

		void get_projection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.projection());
		}

		void get_inverse_projection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.inverse_projection());
		}

		void get_view_projection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.view_projection());
		}
		void get_inverse_view_projection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.inverse_view_projection());
		}

		void get_up_vector(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			math::v3 *const up_vector{ (math::v3* const )data };
			assert(sizeof(up_vector) == size);
			DirectX::XMStoreFloat3(up_vector, camera.up());
		}

		void get_field_of_view(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::PERSPECTIVE);
			f32* const fov{ (f32* const)data };
			assert(sizeof(f32) == size);
			*fov = camera.field_of_view();
		}

		constexpr void get_aspect_ratio(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::PERSPECTIVE);
			f32* const aspect_ratio{ (f32* const)data };
			assert(sizeof(f32) == size);
			*aspect_ratio = camera.aspect_ratio();
		}

		constexpr void get_view_width(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::ORTOGRAPHIC);
			f32* const width{ (f32* const)data };
			assert(sizeof(f32) == size);
			*width = camera.view_width();
		}

		constexpr void get_view_height(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			assert(camera.projection_type() == graphics::Camera::ORTOGRAPHIC);
			f32* const height{ (f32* const)data };
			assert(sizeof(f32) == size);
			*height = camera.view_height();
		}

		constexpr void get_near_z(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			f32* const near_z{ (f32* const)data };
			assert(sizeof(f32) == size);
			*near_z = camera.near_z();
		}

		constexpr void get_far_z(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			f32* const far_z{ (f32* const)data };
			assert(sizeof(f32) == size);
			*far_z = camera.far_z();
		}

		constexpr void get_projection_type(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			graphics::Camera::Type* const type{ (graphics::Camera::Type* const)data };
			assert(sizeof(graphics::Camera::Type) == size);
			*type = camera.projection_type();
		}

		constexpr void get_entity_id(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size) {
			id::id_type* const id{ (id::id_type* const)data };
			assert(sizeof(id::id_type) == size);
			*id = camera.entity_id();
		}

		constexpr void empty_set(D3D12Camera&, const void* const, u32) {}

		using set_function = void(*)(D3D12Camera&, const void* const, u32);
		using get_function = void(*)(const D3D12Camera&, void* const, u32);

		constexpr set_function set_functions[]{
			set_up_vector,
			set_field_of_view,
			set_aspect_ratio,
			set_view_width,
			set_view_height,
			set_near_z,
			set_far_z,
			empty_set,
			empty_set,
			empty_set,
			empty_set,
			empty_set,
			empty_set,
			empty_set
		};

		static_assert(_countof(set_functions) == graphics::CameraParameter::count);

		constexpr get_function get_functions[]{
			get_up_vector,
			get_field_of_view,
			get_aspect_ratio,
			get_view_width,
			get_view_height,
			get_near_z,
			get_far_z,
			get_view,
			get_projection,
			get_inverse_projection,
			get_view_projection,
			get_inverse_view_projection,
			get_projection_type,
			get_entity_id
		};

		static_assert(_countof(get_functions) == graphics::CameraParameter::count);
	}

	D3D12Camera::D3D12Camera(CameraInitInfo info) : _up{ DirectX::XMLoadFloat3(&info.up) }, _near_z{ info.near_z }, _far_z{ info.far_z }, _field_of_view{ info.field_of_view }, _aspect_ratio{ info.aspect_ratio }, _projection_type{ info.type }, _entity_id{ info.entity_id }, _is_dirty{ true } {
		assert(id::is_valid(_entity_id));
		update();
	}

	void D3D12Camera::update() {
		game_entity::Entity entity{ game_entity::entity_id{ _entity_id } };
		using namespace DirectX;
		math::v3 pos{ entity.transform().position() };
		math::v3 dir{ entity.transform().orientation() };
		_position = XMLoadFloat3(&pos);
		_direction = XMLoadFloat3(&dir);
		_view = XMMatrixLookToRH(_position, _direction, _up);

		if (_is_dirty) {
			_projection = (_projection_type == graphics::Camera::PERSPECTIVE) ? XMMatrixPerspectiveFovRH(_field_of_view * XM_PI, _aspect_ratio, _far_z, _near_z) : XMMatrixOrthographicRH(_view_width, _view_height, _far_z, _near_z);
			_inverse_projection = XMMatrixInverse(nullptr, _projection);
			_is_dirty = false;
		}

		_view_projection = XMMatrixMultiply(_view, _projection);
		_inverse_view_projection = XMMatrixInverse(nullptr, _view_projection);
	}

	void D3D12Camera::up(math::v3 up) {
		_up = DirectX::XMLoadFloat3(&up);
	}

	constexpr void D3D12Camera::field_of_view(f32 fov) {
		assert(_projection_type == graphics::Camera::PERSPECTIVE);
		_field_of_view = fov;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::aspect_ratio(f32 aspect_ratio) {
		assert(_projection_type == graphics::Camera::PERSPECTIVE);
		_aspect_ratio = aspect_ratio;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::view_width(f32 width) {
		assert(width);
		assert(_projection_type == graphics::Camera::ORTOGRAPHIC);
		_view_width = width;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::view_height(f32 height) {
		assert(height);
		assert(_projection_type == graphics::Camera::ORTOGRAPHIC);
		_view_height = height;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::near_z(f32 near_z) {
		_near_z = near_z;
		_is_dirty = true;
	}

	constexpr void D3D12Camera::far_z(f32 far_z) {
		_far_z = far_z;
		_is_dirty = true;
	}

	graphics::Camera create(CameraInitInfo info) {
		return graphics::Camera{ camera_id{ cameras.add(info) } };
	}

	void remove(camera_id id) {
		assert(id::is_valid(id));
		cameras.remove(id);
	}

	void set_parameter(camera_id id, CameraParameter::Parameter parameter, const void* const data, u32 data_size) {
		assert(data && data_size);
		assert(parameter < CameraParameter::count);
		D3D12Camera& camera{ get(id) };
		set_functions[parameter](camera, data, data_size);
	}

	void get_parameter(camera_id id, CameraParameter::Parameter parameter, void* const data, u32 data_size) {
		assert(data && data_size);
		assert(parameter < CameraParameter::count);
		D3D12Camera& camera{ get(id) };
		get_functions[parameter](camera, data, data_size);
	}

	D3D12Camera& get(camera_id id) {
		assert(id::is_valid(id));
		return cameras[id];
	}
}