#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12::camera {
	class D3D12Camera {
		public:
			explicit D3D12Camera(CameraInitInfo);

			void update();
			void up(math::v3 up);
			constexpr void field_of_view(f32 fov);
			constexpr void aspect_ratio(f32 aspect_ratio);
			constexpr void view_width(f32 width);
			constexpr void view_height(f32 height);
			constexpr void near_z(f32 near_z);
			constexpr void far_z(f32 far_z);

			[[nodiscard]] constexpr DirectX::XMMATRIX view() const { return _view; }
			[[nodiscard]] constexpr DirectX::XMMATRIX projection() const { return _projection; }
			[[nodiscard]] constexpr DirectX::XMMATRIX inverse_projection() const { return _inverse_projection; }
			[[nodiscard]] constexpr DirectX::XMMATRIX view_projection() const { return _view_projection; }
			[[nodiscard]] constexpr DirectX::XMMATRIX inverse_view_projection() const { return _inverse_view_projection; }
			[[nodiscard]] constexpr DirectX::XMVECTOR up() const { return _up; }
			[[nodiscard]] constexpr DirectX::XMVECTOR position() const { return _position; }
			[[nodiscard]] constexpr DirectX::XMVECTOR direction() const { return _direction; }
			[[nodiscard]] constexpr f32 near_z() const { return _near_z; }
			[[nodiscard]] constexpr f32 far_z() const { return _far_z; }
			[[nodiscard]] constexpr f32 field_of_view() const { return _field_of_view; }
			[[nodiscard]] constexpr f32 aspect_ratio() const { return _aspect_ratio; }
			[[nodiscard]] constexpr f32 view_width() const { return _view_width; }
			[[nodiscard]] constexpr f32 view_height() const { return _view_height; }
			[[nodiscard]] constexpr graphics::Camera::Type projection_type() const { return _projection_type; }
			[[nodiscard]] constexpr id::id_type entity_id() const { return _entity_id; }

		private:
			DirectX::XMMATRIX _view;
			DirectX::XMMATRIX _projection;
			DirectX::XMMATRIX _inverse_projection;
			DirectX::XMMATRIX _view_projection;
			DirectX::XMMATRIX _inverse_view_projection;
			DirectX::XMVECTOR _position{};
			DirectX::XMVECTOR _direction{};
			DirectX::XMVECTOR _up;
			f32 _near_z;
			f32 _far_z;
			union {
				f32 _field_of_view;
				f32 _view_width;
			};
			union {
				f32 _aspect_ratio;
				f32 _view_height;
			};
			graphics::Camera::Type _projection_type;
			id::id_type _entity_id;
			bool _is_dirty;
	};

	graphics::Camera create(CameraInitInfo info);
	void remove(camera_id id);
	void set_parameter(camera_id id, CameraParameter::Parameter parameter, const void* const data, u32 data_size);
	void get_parameter(camera_id id, CameraParameter::Parameter parameter, void* const data, u32 data_size);
	[[nodiscard]] D3D12Camera& get(camera_id id);
}