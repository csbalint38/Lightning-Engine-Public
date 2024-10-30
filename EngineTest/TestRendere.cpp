#include "Platform/Platform.h"
#include "Platform/PlatformTypes.h"
#include "Graphics/Renderer.h"
#include "Graphics/Direct3D12/Direct3D12Core.h"
#include "Content/ContentToEngine.h"
#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"
#include "Components/Geometry.h"
#include "Input/Input.h"
#include "TestRenderer.h"
#include "ShaderCompilation.h"

#include <filesystem>
#include <fstream>

#if TEST_RENDERER
	using namespace lightning;

	#pragma region TEST_MULTITHREDING

	#define ENABLE_TEST_WORKERS 0

	constexpr u32 num_threds{ 8 };
	bool shutdown{ false };
	std::thread workers[num_threds];
	
	util::vector<u8> buffer(1024 * 1024, 0);

	void buffer_test_worker() {
		while (!shutdown) {
			auto* resource = graphics::direct3d12::d3dx::create_buffer(buffer.data(), (u32)buffer.size());
			graphics::direct3d12::core::deferred_release(resource);
		}
	}

	template<class FnPtr, class... Args> void init_test_workers(FnPtr&& fn_ptr, Args&&... args) {
		#if ENABLE_TEST_WORKERS
		shutdown = false;
		for (auto& w : workers) {
			w = std::thread(std::forward<FnPtr>(fn_ptr), std::forward<Args>(args)...);
		}
		#endif
	}

	void joint_test_workers() {
		#if ENABLE_TEST_WORKERS
		shutdown = true;

		for (auto& w : workers) w.join();
		#endif
	}

	#pragma endregion
	
	struct CameraSurface {
		game_entity::Entity entity{};
		graphics::Camera camera{};
		graphics::RenderSurface surface{};
	};

	CameraSurface _surfaces[4]{};
	TimeIt timer;

	bool resized{ false };
	bool is_restarting{ false };

	util::vector<id::id_type> render_item_id_cache;

	void destroy_camera_surface(CameraSurface& surface);
	bool test_initialize();
	void test_shutdown();
	void create_render_items();
	void destroy_render_items();
	void generate_lights();
	void remove_lights();
	void test_lights(f32 dt);

	LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

		bool toggle_fullscreen{ false };

		switch (msg) {
			case WM_DESTROY: {
				bool all_closed{ true };
				for (u32 i{ 0 }; i < _countof(_surfaces); ++i) {
					if (_surfaces[i].surface.window.is_valid()) {
						if (_surfaces[i].surface.window.is_closed()) {
							destroy_camera_surface(_surfaces[i]);
						}
						else {
							all_closed = false;
						}
					}
				}
				if (all_closed && !is_restarting) {
					PostQuitMessage(0);
					return 0;
				}
			}
			break;
			case WM_SIZE:
				resized = (wparam != SIZE_MINIMIZED);
				break;

			case WM_SYSCHAR:
				toggle_fullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
				break;

			case WM_KEYDOWN:
				if (wparam == VK_ESCAPE) {
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					return 0;
				}
				else if (wparam == VK_F11) {
					is_restarting = true;
					test_shutdown();
					test_initialize();
				}
		}

		if ((resized && GetKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen) {
			platform::Window win{ platform::window_id{(id::id_type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
			for (u32 i{ 0 }; i < _countof(_surfaces); ++i) {
				if (win.get_id() == _surfaces[i].surface.window.get_id()) {
					if (toggle_fullscreen) {
						win.set_fullscreen(!win.is_fullscreen());
						return 0;
					}
					else {
						_surfaces[i].surface.surface.resize(win.width(), win.height());
						_surfaces[i].camera.aspect_ratio((f32)win.width() / win.height());
						resized = false;
					}
					break;
				}
			}
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	game_entity::Entity create_one_game_entity(math::v3 position, math::v3 rotation, geometry::InitInfo* geometry_info, const char* script_name) {
		transform::InitInfo transform_info{};
		DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&rotation)) };
		math::v4a rot_quat;
		DirectX::XMStoreFloat4A(&rot_quat, quat);
		memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));
		memcpy(&transform_info.position[0], &position.x, sizeof(transform_info.position));

		script::InitInfo script_info{};
		if (script_name) {
			script_info.script_creator = script::detail::get_script_creator_from_engine(script::detail::string_hash()(script_name));
			assert(script_info.script_creator);
		}

		game_entity::EntityInfo entity_info{};
		entity_info.transform = &transform_info;
		entity_info.script = &script_info;
		entity_info.geometry = geometry_info;
		game_entity::Entity entity{ game_entity::create(entity_info) };
		assert(entity.is_valid());
		return entity;
	}

	void remove_game_entity(game_entity::entity_id id) { game_entity::remove(id); }

	bool read_file(std::filesystem::path path, std::unique_ptr<u8[]>& data, u64& size) {
		if (!std::filesystem::exists(path)) return false;
		size = std::filesystem::file_size(path);
		assert(size);
		if (!size) return false;
		data = std::make_unique<u8[]>(size);
		std::ifstream file{ path, std::ios::in | std::ios::binary };

		if (!file || !file.read((char*)data.get(), size)) {
			file.close();
			return false;
		}

		file.close();

		return true;
	}

	void create_camera_surface(CameraSurface& surface, platform::WindowInitInfo info) {
		surface.surface.window = platform::create_window(&info);
		surface.surface.surface = graphics::create_surface(surface.surface.window);
		surface.entity = create_one_game_entity({ 0, 0, 0 }, { 0, 3.14f, 0 }, nullptr, "CameraScript");
		surface.camera = graphics::create_camera(graphics::PerspectiveCameraInitInfo{ surface.entity.get_id() });
		surface.camera.aspect_ratio((f32)surface.surface.window.width() / surface.surface.window.height());
	}

	void destroy_camera_surface(CameraSurface& surface) {
		CameraSurface temp{ surface };
		surface = {};
		if (temp.surface.surface.is_valid()) graphics::remove_surface(temp.surface.surface.get_id());
		if (temp.surface.window.is_valid()) platform::remove_window(temp.surface.window.get_id());
		if (temp.camera.is_valid()) graphics::remove_camera(temp.camera.get_id());
		if (temp.entity.is_valid()) game_entity::remove(temp.entity.get_id());
	}

	bool test_initialize() {
		while (!compile_shaders()) {
			if (MessageBox(nullptr, "Failed to compile engine shaders", "Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY) {
				return false;
			}
		}

		if (!graphics::initialize(graphics::GraphicsPlatform::DIRECT3D12)) return false;

		platform::WindowInitInfo info[]{
			{&win_proc, nullptr, L"TestWindow1", 100, 100, 400, 400},
			{&win_proc, nullptr, L"TestWindow2", 150, 150, 800, 400},
			{&win_proc, nullptr, L"TestWindow3", 200, 200, 400, 400},
			{&win_proc, nullptr, L"TestWindow4", 250, 250, 800, 600}
		};
		static_assert(_countof(_surfaces) == _countof(info));

		for (u32 i{ 0 }; i < _countof(_surfaces); ++i) {
			create_camera_surface(_surfaces[i], info[i]);
		}

		init_test_workers(buffer_test_worker);

		create_render_items();

		generate_lights();

		render_item_id_cache.resize(4 + 12);
		geometry::get_render_item_ids(render_item_id_cache.data(), (u32)render_item_id_cache.size());

		input::InputSource source{};
		source.binding = std::hash<std::string>()("move");
		source.source_type = input::InputSource::KEYBOARD;
		source.code = input::InputCode::KEY_A;
		source.multiplier = 1.f;
		source.axis = input::Axis::X;
		input::bind(source);

		source.code = input::InputCode::KEY_D;
		source.multiplier = -1.f;
		input::bind(source);

		source.code = input::InputCode::KEY_W;
		source.multiplier = 1.f;
		source.axis = input::Axis::Z;
		input::bind(source);

		source.code = input::InputCode::KEY_S;
		source.multiplier = -1.f;
		input::bind(source);

		source.code = input::InputCode::KEY_Q;
		source.multiplier = -1.f;
		source.axis = input::Axis::Y;
		input::bind(source);

		source.code = input::InputCode::KEY_E;
		source.multiplier = 1.f;
		input::bind(source);

		is_restarting = false;

		return true;
	}

	void test_shutdown() {
		input::unbind(std::hash<std::string>()("move"));
		remove_lights();
		destroy_render_items();
		joint_test_workers();

		for (u32 i{ 0 }; i < _countof(_surfaces); ++i) {
			destroy_camera_surface(_surfaces[i]);
		}
		graphics::shutdown();
	}

	bool EngineTest::initialize() { return test_initialize(); }

	void EngineTest::run() {

		static u32 counter{ 0 };
		static u32 light_set_key{ 0 };
		++counter;
		//if ((counter % 90) == 0) light_set_key = (light_set_key + 1) % 2;

		timer.begin();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		const f32 dt{ timer.dt_avg() };
		script::update(dt);
		//test_lights(dt);
		for (u32 i{ 0 }; i < _countof(_surfaces); ++i) {
			if (_surfaces[i].surface.surface.is_valid()) {

				f32 thresholds[4 + 12]{};

				graphics::FrameInfo info{};
				info.render_item_ids = render_item_id_cache.data();
				info.render_item_count = 4 + 12;
				info.thresholds = &thresholds[0];
				info.light_set_key = light_set_key;
				info.average_frame_time = dt;
				info.camera_id = _surfaces[i].camera.get_id();

				assert(_countof(thresholds) >= info.render_item_count);
				_surfaces[i].surface.surface.render(info);
			}
		}
		timer.end();
	}

	void EngineTest::shutdown() { test_shutdown(); }
#endif