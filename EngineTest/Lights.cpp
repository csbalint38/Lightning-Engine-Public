#include "EngineAPI/GameEntity.h"
#include "EngineAPI/Light.h"
#include "EngineAPI/TransformComponent.h"
#include "Components/Geometry.h"
#include "Graphics/Renderer.h"

#define RANDOM_LIGHTS 0

using namespace lightning;

game_entity::Entity create_one_game_entity(math::v3 position, math::v3 rotation, geometry::InitInfo* geometry_info, const char* script_name);
void remove_game_entity(game_entity::entity_id id);

namespace {
	const u64 left_set{ 0 };
	const u64 right_set{ 1 };
	constexpr f32 inv_rand_max{ 1.f / RAND_MAX };

	util::vector<graphics::Light> lights;
	util::vector<graphics::Light> disabled_lights;

	constexpr math::v3 rgb_to_color(u8 r, u8 g, u8 b) { return { r / 255.f, g / 255.f, b / 255.f }; }
	f32 random(f32 min = 0.f) { return std::max(min, rand() * inv_rand_max); }

	void create_light(math::v3 position, math::v3 rotation, graphics::Light::Type type, u64 light_set_key) {
		const char* script_name{ nullptr }; //{ type == graphics::Light::SPOT ? "RotatorScript" : nullptr };
		game_entity::entity_id entity_id{ create_one_game_entity(position, rotation, nullptr, script_name).get_id() };

		graphics::LightInitInfo info{};
		info.entity_id = entity_id;
		info.type = type;
		info.light_set_key = light_set_key;
		info.intensity = 1.f;
		info.color = { random(.2f), random(.2f), random(.2f) };
		
		#if RANDOM_LIGHTS
		if (type == graphics::Light::POINT) {
			info.point_params.range = random(.5f) * 2.f;
			info.point_params.attenuation = { 1, 1, 1 };
		}
		else if (type == graphics::Light::SPOT) {
			info.spot_params.range = random(.5f) * 2.f;
			info.spot_params.umbra = (random(.5f) - .4f) * math::PI;
			info.spot_params.penumbra = info.spot_params.umbra + (.1f * math::PI);
			info.spot_params.attenuation = { 1, 1, 1 };
		}
		#else
		if (type == graphics::Light::POINT) {
			info.point_params.range = 1.f;
			info.point_params.attenuation = { 1, 1, 1 };
		}
		else if (type == graphics::Light::SPOT) {
			info.spot_params.range = 2.f;
			info.spot_params.umbra = .1f * math::PI;
			info.spot_params.penumbra = info.spot_params.umbra + (.1f * math::PI);
			info.spot_params.attenuation = { 1, 1, 1 };
		}
		#endif

		graphics::Light light{ graphics::create_light(info) };
		assert(light.is_valid());
		lights.push_back(light);
	}
}

void generate_lights() {
	graphics::create_light_set(left_set);
	graphics::create_light_set(right_set);

	graphics::LightInitInfo info{};
	info.entity_id = create_one_game_entity({}, { 0, 0, 0 }, nullptr, nullptr).get_id();
	info.type = graphics::Light::DIRECTIONAL;
	info.light_set_key = left_set;
	info.intensity = 1.f;
	info.color = rgb_to_color(174, 174, 174);

	lights.emplace_back(graphics::create_light(info));

	info.entity_id = create_one_game_entity({}, { math::PI * .5f, 0, 0 }, nullptr, nullptr).get_id();
	info.color = rgb_to_color(17, 27, 48);
	lights.emplace_back(graphics::create_light(info));

	info.entity_id = create_one_game_entity({}, { -math::PI * .5f, 0, 0 }, nullptr, nullptr).get_id();
	info.color = rgb_to_color(63, 47, 30);
	lights.emplace_back(graphics::create_light(info));

	info.entity_id = create_one_game_entity({}, { 0, 0, 0 }, nullptr, nullptr).get_id();
	info.light_set_key = right_set;
	info.color = rgb_to_color(150, 100, 200);
	lights.emplace_back(graphics::create_light(info));

	info.entity_id = create_one_game_entity({}, { math::PI * .5f, 0, 0 }, nullptr, nullptr).get_id();
	info.color = rgb_to_color(17, 27, 48);
	lights.emplace_back(graphics::create_light(info));

	info.entity_id = create_one_game_entity({}, { -math::PI * .5f, 0, 0 }, nullptr, nullptr).get_id();
	info.color = rgb_to_color(63, 47, 130);
	lights.emplace_back(graphics::create_light(info));

	#if !RANDOM_LIGHTS
	create_light({ 0, 3, 0 }, {}, graphics::Light::POINT, left_set);
	create_light({ 0, -.4f, 1.f }, {}, graphics::Light::POINT, left_set);
	create_light({ -1.5f, 2.3f, 2.5f }, {}, graphics::Light::POINT, left_set);
	create_light({ 0, 2.3f, 5 }, { 0, 3.14f, 0 }, graphics::Light::SPOT, left_set);
	#else
	srand(17);

	constexpr f32 scale1{ 2 };
	constexpr math::v3 scale{ 1.f * scale1, .5f * scale1, 1.f * scale1};
	constexpr s32 dim{ 10 };
	for (s32 x{ -dim }; x < dim; ++x) {
		for (s32 y{ 0 }; y < 2 * dim; ++y) {
			for (s32 z{ -dim }; z < dim; ++z) {
				create_light({ (f32)(x * scale.x), (f32)(y * scale.y), (f32)(z * scale.z) }, { random() * 3.14f, random() * 3.14f,  random() * 3.14f }, random() > .5f ? graphics::Light::SPOT : graphics::Light::POINT, left_set);
				create_light({ (f32)(x * scale.x), (f32)(y * scale.y), (f32)(z * scale.z) }, { random() * 3.14f, random() * 3.14f,  random() * 3.14f }, random() > .5f ? graphics::Light::SPOT : graphics::Light::POINT, right_set);
			}
		}
	}
	#endif
}

void remove_lights() {
	for (auto& light : lights) {
		const game_entity::entity_id id{ light.entity_id() };
		graphics::remove_light(light.get_id(), light.light_set_key());
		remove_game_entity(id);
	}

	for (auto& light : disabled_lights) {
		const game_entity::entity_id id{ light.entity_id() };
		graphics::remove_light(light.get_id(), light.light_set_key());
		remove_game_entity(id);
	}

	lights.clear();
	disabled_lights.clear();

	graphics::remove_light_set(left_set);
	graphics::remove_light_set(right_set);

}

void test_lights(f32 dt) {
	#if 0
	static f32 t{ 0 };
	t += .05f;

	for (u32 i{ 0 }; i < (u32)lights.size(); ++i) {
		f32 sine{ DirectX::XMScalarSin(t + lights[i].get_id()) };
		sine *= sine;
		lights[i].intensity(2.f * sine);
	}
	#else

	u32 count{ (u32)(random(.1f) * 100) };
	for (u32 i{ 0 }; i < count; ++i) {
		if (!lights.size()) break;
		const u32 index{ (u32)(random() * (lights.size() - 1)) };
		graphics::Light light{ lights[index] };
		light.is_enabled(false);
		util::erease_unordered(lights, index);
		disabled_lights.emplace_back(light);
	}

	count = (u32)(random(.1f) * 50);
	for (u32 i{ 0 }; i < count; ++i) {
		if (!lights.size()) break;
		const u32 index{ (u32)(random() * (lights.size() - 1)) };
		graphics::Light light{ lights[index] };
		const game_entity::entity_id id{ light.entity_id() };
		graphics::remove_light(light.get_id(), light.light_set_key());
		remove_game_entity(id);
		util::erease_unordered(lights, index);
	}

	count = (u32)(random(.1f) * 50);
	for (u32 i{ 0 }; i < count; ++i) {
		if (!disabled_lights.size()) break;
		const u32 index{ (u32)(random() * (disabled_lights.size() - 1)) };
		graphics::Light light{ disabled_lights[index] };
		const game_entity::entity_id id{ light.entity_id() };
		graphics::remove_light(light.get_id(), light.light_set_key());
		remove_game_entity(id);
		util::erease_unordered(disabled_lights, index);
	}

	count = (u32)(random(.1f) * 100);
	for (u32 i{ 0 }; i < count; ++i) {
		if (!disabled_lights.size()) break;
		const u32 index{ (u32)(random() * (disabled_lights.size() - 1)) };
		graphics::Light light{ disabled_lights[index] };
		light.is_enabled(true);
		util::erease_unordered(disabled_lights, index);
		lights.emplace_back(light);
	}

	constexpr f32 scale1{ 1 };
	constexpr math::v3 scale{ 1.f * scale1, .5f * scale1, 1.f * scale1 };
	count = (u32)(random(.1f) * 50);
	for (u32 i{ 0 }; i < count; ++i) {
		math::v3 p1{ (random() * 2 - 1.f) * 13.f * scale.x, random() * 2 * 13.f * scale.y, (random() * 2 - 1.f) * 13.f * scale.z };
		math::v3 p2{ (random() * 2 - 1.f) * 13.f * scale.x, random() * 2 * 13.f * scale.y, (random() * 2 - 1.f) * 13.f * scale.z };
		create_light(p1, { random() * 3.14f, random() * 3.14f, random() * 3.14f }, random() > .5f ? graphics::Light::SPOT : graphics::Light::POINT, left_set);
		create_light(p2, { random() * 3.14f, random() * 3.14f, random() * 3.14f }, random() > .5f ? graphics::Light::SPOT :graphics::Light::POINT,right_set);
	}
	#endif
}