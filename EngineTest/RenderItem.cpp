#include "CommonHeaders.h"
#include "Content/ContentToEngine.h"
#include "Graphics/Renderer.h"
#include "ShaderCompilation.h"
#include "Components/Entity.h"
#include "Components/Geometry.h"
#include "../ContentTools/Geometry.h"
#include "Test.h"

#include <filesystem>

#undef OPAQUE

#if TEST_RENDERER
using namespace lightning;

game_entity::Entity create_one_game_entity(math::v3 position, math::v3 rotation, geometry::InitInfo* geometry_info, const char* script_name);
void remove_game_entity(game_entity::entity_id id);

bool read_file(std::filesystem::path, std::unique_ptr<u8[]>&, u64&);

namespace {
	id::id_type building_model_id{ id::invalid_id };
	id::id_type fan_model_id{ id::invalid_id };
	id::id_type blades_model_id{ id::invalid_id };
	id::id_type fembot_model_id{ id::invalid_id };
	id::id_type sphere_model_id{ id::invalid_id };

	game_entity::entity_id building_entity_id{ id::invalid_id };
	game_entity::entity_id fan_entity_id{ id::invalid_id };
	game_entity::entity_id blades_entity_id{ id::invalid_id };
	game_entity::entity_id fembot_entity_id{ id::invalid_id };
	game_entity::entity_id sphere_entity_ids[12];

	struct TextureUsage {
		enum Usage : u32 {
			AMBIENT_OCCLUSIN = 0,
			BASE_COLOR,
			EMISSIVE,
			METAL_ROUGH,
			NORMAL,

			count
		};
	};

	id::id_type texture_ids[TextureUsage::count];

	id::id_type vs_id{ id::invalid_id };
	id::id_type ps_id{ id::invalid_id };
	id::id_type textured_ps_id{ id::invalid_id };
	id::id_type default_material_id{ id::invalid_id };
	id::id_type fembot_material_id{ id::invalid_id };
	id::id_type pbr_material_ids[12];

	[[nodiscard]] id::id_type load_asset(const char* path, content::AssetType::Type type) {
		std::unique_ptr<u8[]> buffer;
		u64 size{ 0 };
		read_file(path, buffer, size);

		const id::id_type asset_id{ content::create_resource(buffer.get(), type) };
		assert(id::is_valid(asset_id));

		return asset_id;
	}

	[[nodiscard]] id::id_type load_model(const char* path) {
		return load_asset(path, content::AssetType::MESH);
	}

	[[nodiscard]] id::id_type load_texture(const char* path) {
		return load_asset(path, content::AssetType::TEXTURE);
	}

	void load_shaders() {
		ShaderFileInfo info{};
		info.file_name = "TestShader.hlsl";
		info.function = "test_shader_vs";
		info.type = ShaderType::VERTEX;

		const char* shader_path{ "../../EngineTest/" };

		std::wstring defines[]{ L"ELEMENTS_TYPE=1", L"ELEMENTS_TYPE=3" };
		util::vector<u32> keys;
		keys.emplace_back(tools::elements::ElementsType::STATIC_NORMAL);
		keys.emplace_back(tools::elements::ElementsType::STATIC_NORMAL_TEXTURE);

		util::vector<std::wstring> extra_args{};
		util::vector<std::unique_ptr<u8[]>> vertex_shaders;
		util::vector<const u8*> vertex_shader_pointers;

		for (u32 i{ 0 }; i < _countof(defines); ++i) {
			extra_args.clear();
			extra_args.emplace_back(L"-D");
			extra_args.emplace_back(defines[i]);

			vertex_shaders.emplace_back(std::move(compile_shader(info, shader_path, extra_args)));
			assert(vertex_shaders.back().get());

			vertex_shader_pointers.emplace_back(vertex_shaders.back().get());
		}

		extra_args.clear();
		info.function = "test_shader_ps";
		info.type = ShaderType::PIXEL;
		util::vector<std::unique_ptr<u8[]>> pixel_shaders;

		pixel_shaders.emplace_back(compile_shader(info, shader_path, extra_args));
		assert(pixel_shaders.back().get());

		defines[0] = L"TEXTURED_MTL=1";
		extra_args.emplace_back(L"-D");
		extra_args.emplace_back(defines[0]);

		pixel_shaders.emplace_back(compile_shader(info, shader_path, extra_args));
		assert(pixel_shaders.back().get());

		vs_id = content::add_shader_group(vertex_shader_pointers.data(), vertex_shader_pointers.size(), keys.data());

		const u8* pixel_shader_pointers[]{ pixel_shaders[0].get()};
		ps_id = content::add_shader_group(pixel_shader_pointers, 1, &u32_invalid_id);

		pixel_shader_pointers[0] = pixel_shaders[1].get();
		textured_ps_id = content::add_shader_group(pixel_shader_pointers, 1, &u32_invalid_id);
	}

	void create_material() {
		assert(id::is_valid(vs_id) && id::is_valid(ps_id) && id::is_valid(textured_ps_id));
		graphics::MaterialInitInfo info{};
		info.shader_ids[ShaderType::VERTEX] = vs_id;
		info.shader_ids[ShaderType::PIXEL] = ps_id;
		info.type = graphics::MaterialType::OPAQUE;
		default_material_id = content::create_resource(&info, content::AssetType::MATERIAL);

		memset(pbr_material_ids, 0xff, sizeof(pbr_material_ids));
		math::v2 metal_rough[_countof(pbr_material_ids)]{
			{0.f, 0.f},
			{0.f, 0.2f},
			{0.f, 0.4f},
			{0.f, 0.6f},
			{0.f, 0.8f},
			{0.f, 1.f},
			{1.f, 0.f},
			{1.f, 0.2f},
			{1.f, 0.4f},
			{1.f, 0.6f},
			{1.f, 0.8f},
			{1.f, 1.f},
		};

		graphics::MaterialSurface& s{ info.surface };
		s.base_color = { .5f, .5f, .5f, 1.f };

		for (u32 i{ 0 }; i < _countof(pbr_material_ids); ++i) {
			s.metallic = metal_rough[i].x;
			s.roughness = metal_rough[i].y;
			pbr_material_ids[i] = content::create_resource(&info, content::AssetType::MATERIAL);
		}

		info.shader_ids[ShaderType::PIXEL] = textured_ps_id;
		info.texture_count = TextureUsage::count;
		info.texture_ids = &texture_ids[0];
		fembot_material_id = content::create_resource(&info, content::AssetType::MATERIAL);
	}

	void remove_model(id::id_type model_id) {
		if (id::is_valid(model_id)) {
			content::destroy_resource(model_id, content::AssetType::MESH);
		}
	}
}

void create_render_items() {
	memset(&texture_ids[0], 0xff, sizeof(id::id_type) * _countof(texture_ids));

	std::thread threads[]{
		std::thread{ [] { texture_ids[TextureUsage::AMBIENT_OCCLUSIN] = load_texture("C:/Users/balin/Documents/Lightning-Engine/EngineTest/ambient_occlusion.img"); }},
		std::thread{ [] { texture_ids[TextureUsage::BASE_COLOR] = load_texture("C:/Users/balin/Documents/Lightning-Engine/EngineTest/base_color.img"); }},
		std::thread{ [] { texture_ids[TextureUsage::EMISSIVE] = load_texture("C:/Users/balin/Documents/Lightning-Engine/EngineTest/emissive.img"); }},
		std::thread{ [] { texture_ids[TextureUsage::METAL_ROUGH] = load_texture("C:/Users/balin/Documents/Lightning-Engine/EngineTest/metal_rough.img"); }},
		std::thread{ [] { texture_ids[TextureUsage::NORMAL] = load_texture("C:/Users/balin/Documents/Lightning-Engine/EngineTest/normal.img"); }},

		std::thread{ [] { building_model_id = load_model("C:/Users/balin/Documents/Lightning-Engine/EngineTest/villa.model"); }},
		std::thread{ [] { fan_model_id = load_model("C:/Users/balin/Documents/Lightning-Engine/EngineTest/turbine.model"); }},
		std::thread{ [] { blades_model_id = load_model("C:/Users/balin/Documents/Lightning-Engine/EngineTest/blades.model"); }},
		std::thread{ [] { fembot_model_id = load_model("C:/Users/balin/Documents/Lightning-Engine/EngineTest/fembot.model"); }},
		std::thread{ [] { sphere_model_id = load_model("C:/Users/balin/Documents/Lightning-Engine/EngineTest/sphere.model"); }},
		std::thread{ [] { load_shaders(); } },
	};

	for (auto& t : threads) {
		t.join();
	}

	create_material();
	id::id_type materials[]{ default_material_id };
	id::id_type fembot_materials[]{ fembot_material_id, fembot_material_id };

	geometry::InitInfo geometry_info{};
	geometry_info.material_count = _countof(materials);
	geometry_info.material_ids = &materials[0];

	geometry_info.geometry_content_id = building_model_id;
	building_entity_id = create_one_game_entity({}, {}, &geometry_info, nullptr).get_id();

	geometry_info.geometry_content_id = fan_model_id;
	fan_entity_id = create_one_game_entity({ 0, 0, 69.78f }, {}, &geometry_info, nullptr).get_id();

	geometry_info.geometry_content_id = blades_model_id;
	blades_entity_id = create_one_game_entity({ -.152f, 60.555f, 66.362f }, {}, &geometry_info, "TurbineScript").get_id();

	geometry_info.geometry_content_id = fembot_model_id;
	geometry_info.material_count = _countof(fembot_materials);
	geometry_info.material_ids = &fembot_materials[0];
	fembot_entity_id = create_one_game_entity({ -6.f, 0.f, 10.f }, { 0.f, math::PI, 0.f }, &geometry_info, "RotatorScript").get_id();

	geometry_info.geometry_content_id = sphere_model_id;
	geometry_info.material_count = 1;

	for (u32 i{ 0 }; i < _countof(sphere_entity_ids); ++i) {
		id::id_type id{ pbr_material_ids[i] };
		id::id_type sphere_materials[]{ id };
		geometry_info.material_ids = &sphere_materials[0];
		const f32 x{ -6.f + i % 6 };
		const f32 y{ (i < 6) ? 7.f : 5.5f };
		const f32 z = x;
		sphere_entity_ids[i] = create_one_game_entity({ x, y, z }, {}, &geometry_info, nullptr).get_id();
	}
}

void destroy_render_items() {
	remove_game_entity(building_entity_id);
	remove_game_entity(fan_entity_id);
	remove_game_entity(blades_entity_id);
	remove_game_entity(fembot_entity_id);

	for (u32 i{ 0 }; i < _countof(sphere_entity_ids); ++i) {
		remove_game_entity(sphere_entity_ids[i]);
	}

	remove_model(building_model_id);
	remove_model(fan_model_id);
	remove_model(blades_model_id);
	remove_model(fembot_model_id);
	remove_model(sphere_model_id);

	if (id::is_valid(default_material_id)) content::destroy_resource(default_material_id, content::AssetType::MATERIAL);
	if (id::is_valid(fembot_material_id)) content::destroy_resource(fembot_material_id, content::AssetType::MATERIAL);

	for (id::id_type id : pbr_material_ids) {
		if (id::is_valid(id)) {
			content::destroy_resource(id, content::AssetType::MATERIAL);
		}
	}

	for (id::id_type id : texture_ids) {
		if (id::is_valid(id)) {
			content::destroy_resource(id, content::AssetType::TEXTURE);
		}
	}

	if (id::is_valid(vs_id)) content::remove_shader_group(vs_id);
	if (id::is_valid(ps_id)) content::remove_shader_group(ps_id);
	if (id::is_valid(textured_ps_id)) content::remove_shader_group(textured_ps_id);

}

#endif