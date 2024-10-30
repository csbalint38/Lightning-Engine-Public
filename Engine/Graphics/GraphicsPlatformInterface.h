#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"
#include "Platform/Window.h"

namespace lightning::graphics {
	struct PlatformInterface {
		bool(*initialize)(void);
		void(*shutdown)(void);

		struct {
			Surface(*create)(platform::Window);
			void(*remove)(surface_id);
			void(*resize)(surface_id, u32, u32);
			u32(*width)(surface_id);
			u32(*height)(surface_id);
			void(*render)(surface_id, FrameInfo);
		} surface;

		struct {
			void(*create_light_set)(u64);
			void(*remove_light_set)(u64);
			Light(*create)(LightInitInfo);
			void(*remove)(light_id, u64);
			void(*set_parameter)(light_id, u64, LightParameter::Parameter, const void* const, u32);
			void(*get_parameter)(light_id, u64, LightParameter::Parameter, void* const, u32);
		} light;

		struct {
			Camera(*create)(CameraInitInfo);
			void(*remove)(camera_id);
			void(*set_parameter)(camera_id, CameraParameter::Parameter, const void* const, u32);
			void(*get_parameter)(camera_id, CameraParameter::Parameter, void* const, u32);
		} camera;

		struct {
			id::id_type(*add_submesh)(const u8*&);
			void(*remove_submesh)(id::id_type);
			id::id_type(*add_texture)(const u8* const);
			void(*remove_texture)(id::id_type);
			id::id_type(*add_material)(MaterialInitInfo);
			void(*remove_material)(id::id_type);
			id::id_type(*add_render_item)(id::id_type, id::id_type, u32, const id::id_type* const);
			void(*remove_render_item)(id::id_type);
		} resources;

		GraphicsPlatform platform = (GraphicsPlatform)-1;
	};
}