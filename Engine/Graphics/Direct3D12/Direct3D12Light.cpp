#include "Direct3D12Light.h"
#include "Direct3D12Core.h"
#include "Shaders/ShaderTypes.h"
#include "EngineAPI/GameEntity.h"
#include "Components/Transform.h"

namespace lightning::graphics::direct3d12::light {
	namespace {

		template<u32 n> struct U32SetBits {
			static_assert(n > 0 && n <= 32);
			constexpr static const u32 bits{ U32SetBits<n - 1>::bits | (1 << (n - 1)) };
		};

		template<> struct U32SetBits<0> {
			constexpr static const u32 bits{ 0 };
		};

		static_assert(U32SetBits<FRAME_BUFFER_COUNT>::bits < (1 << 8), "Frame buffer count is too large. Maximum number of frame buffers is 8");

		constexpr u8 dirty_bits_mask{ (u8)U32SetBits<FRAME_BUFFER_COUNT>::bits };

		struct LightOwner {
			game_entity::entity_id entity_id{ id::invalid_id };
			u32 data_index{ u32_invalid_id };
			graphics::Light::Type type;
			bool is_enabled;
		};

		#if USE_STL_VECTOR
		#define CONSTEXPR
		#else
		#define CONSTEXPR constexpr
		#endif

		class LightSet {
		public:
			constexpr graphics::Light add(const LightInitInfo& info) {
				if (info.type == graphics::Light::DIRECTIONAL) {
					u32 index{ u32_invalid_id };

					for (u32 i{ 0 }; i < _non_cullable_owners.size(); ++i) {
						if (!id::is_valid(_non_cullable_owners[i])) {
							index = i;
							break;
						}
					}

					if (index == u32_invalid_id) {
						index = (u32)_non_cullable_owners.size();
						_non_cullable_owners.emplace_back();
						_non_cullable_lights.emplace_back();
					}

					hlsl::DirectionalLightParameters& params{ _non_cullable_lights[index] };
					params.color = info.color;
					params.intensity = info.intensity;

					LightOwner owner{ game_entity::entity_id{info.entity_id}, index, info.type, info.is_enabled };
					const light_id id{ _owners.add(owner) };
					_non_cullable_owners[index] = id;

					return graphics::Light{ id, info.light_set_key };
				}

				else {
					u32 index{ u32_invalid_id };

					for (u32 i{ _enabled_cullable_light_count }; i < _cullable_owners.size(); ++i) {
						if (!id::is_valid(_cullable_owners[i])) {
							index = i;
							break;
						}
					}

					if (index == u32_invalid_id) {
						index = (u32)_cullable_owners.size();
						_cullable_lights.emplace_back();
						_culling_info.emplace_back();
						_bounding_spheres.emplace_back();
						_cullable_entity_ids.emplace_back();
						_cullable_owners.emplace_back();
						_dirty_bits.emplace_back();
						assert(_cullable_owners.size() == _cullable_lights.size());
						assert(_cullable_owners.size() == _culling_info.size());
						assert(_cullable_owners.size() == _bounding_spheres.size());
						assert(_cullable_owners.size() == _cullable_entity_ids.size());
						assert(_cullable_owners.size() == _dirty_bits.size());
					}

					add_cullable_light_parameters(info, index);
					add_light_culling_info(info, index);
					const light_id id{ _owners.add(LightOwner{game_entity::entity_id{info.entity_id}, index, info.type, info.is_enabled}) };
					_cullable_entity_ids[index] = _owners[id].entity_id;
					_cullable_owners[index] = id;
					make_dirty(index);
					enable(id, info.is_enabled);
					update_transform(index);

					return graphics::Light{ id, info.light_set_key };
				}
			}

			constexpr void remove(light_id id) {
				enable(id, false);

				const LightOwner& owner{ _owners[id] };

				if (owner.type == graphics::Light::DIRECTIONAL) {
					_non_cullable_owners[owner.data_index] = light_id{ id::invalid_id };
				}
				else {
					assert(_owners[_cullable_owners[owner.data_index]].data_index == owner.data_index);
					_cullable_owners[owner.data_index] = light_id{ id::invalid_id };
				}

				_owners.remove(id);
			}

			void update_transforms() {
				for (const auto& id : _non_cullable_owners) {
					if (!id::is_valid(id)) continue;

					const LightOwner& owner{ _owners[id] };
					if (owner.is_enabled) {
						const game_entity::Entity entity{ game_entity::entity_id{ owner.entity_id } };
						hlsl::DirectionalLightParameters& params{ _non_cullable_lights[owner.data_index] };
						params.direction = entity.orientation();
					}
				}

				const u32 count{ _enabled_cullable_light_count };
				if (!count) return;

				assert(_cullable_entity_ids.size() >= count);
				_transform_flags_cache.resize(count);
				transform::get_updated_component_flags(_cullable_entity_ids.data(), count, _transform_flags_cache.data());

				for (u32 i{ 0 }; i < count; ++i) {
					if (_transform_flags_cache[i]) {
						update_transform(i);
					}
				}

			}

			constexpr void enable(light_id id, bool is_enabled) {
				_owners[id].is_enabled = is_enabled;

				if (_owners[id].type == graphics::Light::DIRECTIONAL) {
					return;
				}

				const u32 data_index{ _owners[id].data_index };
				u32& count{ _enabled_cullable_light_count };

				if (is_enabled) {
					if (data_index > count) {
						assert(count < _cullable_lights.size());
						swap_cullable_lights(data_index, count);
						++count;
					}
					else if (data_index == count) {
						++count;
					}
				}
				else if(count > 0) {
					const u32 last{ count - 1 };
					if (data_index < last) {
						swap_cullable_lights(data_index, last);
						--count;
					}
					else if (data_index == last) {
						--count;
					}
				}
			}

			constexpr void intensity(light_id id, f32 intensity) {
				if (intensity < 0.f) intensity = 0.f;

				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == graphics::Light::DIRECTIONAL) {
					assert(index < _non_cullable_lights.size());
					_non_cullable_lights[index].intensity = intensity;
				}
				else {
					assert(_owners[_cullable_owners[index]].data_index == index);
					assert(index < _cullable_lights.size());
					_cullable_lights[index].intensity = intensity;
					make_dirty(index);
				}
			}

			constexpr void color(light_id id, math::v3 color) {
				assert(color.x <= 1.f && color.y <= 1.f && color.z <= 1.f);
				assert(color.x >= 0.f && color.y >= 0.f && color.z >= 0.f);

				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == graphics::Light::DIRECTIONAL) {
					assert(index < _non_cullable_lights.size());
					_non_cullable_lights[index].color = color;
				}
				else {
					assert(_owners[_cullable_owners[index]].data_index == index);
					assert(index < _cullable_lights.size());
					_cullable_lights[index].color = color;
					make_dirty(index);
				}
			}

			CONSTEXPR void attenuation(light_id id, math::v3 attenuation) {
				assert(attenuation.x >= 0.f && attenuation.y >= 0.f && attenuation.z >= 0.f);
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != graphics::Light::DIRECTIONAL);
				assert(index < _cullable_lights.size());
				_cullable_lights[index].attenuation = attenuation;
				make_dirty(index);
			}

			CONSTEXPR void range(light_id id, f32 range) {
				assert(range > 0);
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != graphics::Light::DIRECTIONAL);
				assert(index < _cullable_lights.size());
				_cullable_lights[index].range = range;
				_culling_info[index].range = range;
				#if USE_BOUNDING_SPHERES
				_culling_info[index].cos_penumbra = -1.f;
				#endif
				_bounding_spheres[index].radius = range;
				make_dirty(index);

				if (owner.type == graphics::Light::SPOT) {
					calculate_cone_bounding_sphere(_cullable_lights[index], _bounding_spheres[index]);
					#if USE_BOUNDING_SPHERES
					_culling_info[index].cos_penumbra = _cullable_lights[index].cos_penumbra;
					#else
					_culling_info[index].cone_radius = calculate_cone_radius(range, _cullable_lights[index].cos_penumbra);
					#endif		
				}
			}

			void umbra(light_id id, f32 umbra) {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type == graphics::Light::SPOT);
				assert(index < _cullable_lights.size());

				umbra = math::clamp(umbra, 0.f, math::PI);
				_cullable_lights[index].cos_umbra = DirectX::XMScalarACos(umbra * .5f);
				make_dirty(index);

				if (penumbra(id) < umbra) {
					penumbra(id, umbra);
				}
			}

			void penumbra(light_id id, f32 penumbra) {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type == graphics::Light::SPOT);
				assert(index < _cullable_lights.size());

				penumbra = math::clamp(penumbra, umbra(id), math::PI);
				_cullable_lights[index].cos_penumbra = DirectX::XMScalarACos(penumbra * .5f);
				calculate_cone_bounding_sphere(_cullable_lights[index], _bounding_spheres[index]);

				#if USE_BOUNDING_SPHERES
				_culling_info[index].cos_penumbra = _cullable_lights[index].cos_penumbra;
				#else
				_culling_info[index].cone_radius = calculate_cone_radius(range(id), _cullable_lights[index].cos_penumbra);
				#endif

				make_dirty(index);
			}

			constexpr bool is_enabled(light_id id) const { return _owners[id].is_enabled; }

			constexpr f32 intensity(light_id id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == graphics::Light::DIRECTIONAL) {
					assert(index < _non_cullable_lights.size());
					return _non_cullable_lights[index].intensity;
				}

				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].intensity;
			}

			constexpr math::v3 color(light_id id) const {

				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };

				if (owner.type == graphics::Light::DIRECTIONAL) {
					assert(index < _non_cullable_lights.size());
					return _non_cullable_lights[index].color;
				}


				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].color;
			}

			CONSTEXPR math::v3 attenuation(light_id id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != graphics::Light::DIRECTIONAL);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].attenuation;
			}

			CONSTEXPR f32 range(light_id id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type != graphics::Light::DIRECTIONAL);
				assert(index < _cullable_lights.size());
				return _cullable_lights[index].range;
			}

			f32 umbra(light_id id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type == graphics::Light::SPOT);
				assert(index < _cullable_lights.size());
				return DirectX::XMScalarACos(_cullable_lights[index].cos_umbra) * 2.f;
			}

			f32 penumbra(light_id id) const {
				const LightOwner& owner{ _owners[id] };
				const u32 index{ owner.data_index };
				assert(_owners[_cullable_owners[index]].data_index == index);
				assert(owner.type == graphics::Light::SPOT);
				assert(index < _cullable_lights.size());
				return DirectX::XMScalarACos(_cullable_lights[index].cos_penumbra) * 2.f;
			}

			constexpr graphics::Light::Type type(light_id id) const { return _owners[id].type; }
			constexpr id::id_type entity_id(light_id id) const { return _owners[id].entity_id; }

			CONSTEXPR u32 non_cullable_light_count() const {
				u32 count{ 0 };
				for (const auto& id : _non_cullable_owners) {
					if (id::is_valid(id) && _owners[id].is_enabled) ++count;
				}
				return count;
			}

			CONSTEXPR void non_cullable_lights(hlsl::DirectionalLightParameters* const lights, [[maybe_unused]] u32 buffer_size)const {
				assert(buffer_size >= math::align_size_up<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(non_cullable_light_count() * sizeof(hlsl::DirectionalLightParameters)));

				const u32 count{ (u32)_non_cullable_owners.size() };
				u32 index{ 0 };
				for (u32 i{ 0 }; i < count; ++i) {
					if (!id::is_valid(_non_cullable_owners[i])) continue;

					const LightOwner& owner{ _owners[_non_cullable_owners[i]] };
					if (owner.is_enabled) {
						assert(_owners[_non_cullable_owners[i]].data_index == i);
						lights[index] = _non_cullable_lights[i];
						++index;
					}
				}
			}

			constexpr u32 cullable_light_count() const { return _enabled_cullable_light_count; };
			constexpr bool has_lights() const { return _owners.size() > 0; }

		private:
			f32 calculate_cone_radius(f32 range, f32 cos_penumbra) {
				const f32 sin_penumbra{ sqrt(1.f - cos_penumbra * cos_penumbra) };
			
				return sin_penumbra * range;
			}

			void calculate_cone_bounding_sphere(const hlsl::LightParameters& params, hlsl::Sphere& sphere) {
				using namespace DirectX;

				XMVECTOR tip{ XMLoadFloat3(&params.position) };
				XMVECTOR direction{ XMLoadFloat3(&params.direction) };
				const f32 cone_cos{ params.cos_penumbra };
				assert(cone_cos > 0.f);

				if (cone_cos >= .707107f) {
					sphere.radius = params.range / (2.f * cone_cos);
					XMStoreFloat3(&sphere.center, tip + sphere.radius * direction);
				}
				else {
					XMStoreFloat3(&sphere.center, tip + cone_cos * params.range * direction);
					const f32 cone_sin{ sqrt(1.f - cone_cos * cone_cos) };
					sphere.radius = cone_sin * params.range;
				}
			}

			void update_transform(u32 index) {
				const game_entity::Entity entity{ game_entity::entity_id{_cullable_entity_ids[index]} };
				hlsl::LightParameters& params{ _cullable_lights[index] };
				params.position = entity.position();

				hlsl::LightCullingLightInfo& culling_info{ _culling_info[index] };
				culling_info.position = _bounding_spheres[index].center = params.position;

				if (_owners[_cullable_owners[index]].type == graphics::Light::SPOT) {
					culling_info.direction = params.direction = entity.orientation();
					calculate_cone_bounding_sphere(params, _bounding_spheres[index]);
				}

				make_dirty(index);
 			}

			CONSTEXPR void add_cullable_light_parameters(const LightInitInfo& info, u32 index) {
				using graphics::Light;

				assert(info.type != Light::DIRECTIONAL && index < _cullable_lights.size());

				hlsl::LightParameters& params{ _cullable_lights[index] };
				#if !USE_BOUNDING_SPHERES
				params.type = info.type;
				assert(params.type < Light::count);
				#endif
				params.color = info.color;
				params.intensity = info.intensity;

				if (info.type == Light::POINT) {
					const PointLightParams& p{ info.point_params };
					params.attenuation = p.attenuation;
					params.range = p.range;
				}
				else if (info.type == Light::SPOT) {
					const SpotLightParams& p{ info.spot_params };
					params.attenuation = p.attenuation;
					params.range = p.range;
					params.cos_umbra = DirectX::XMScalarCos(p.umbra * .5f);
					params.cos_penumbra = DirectX::XMScalarCos(p.penumbra * .5f);
				}
			}

			CONSTEXPR void add_light_culling_info(const LightInitInfo& info, u32 index) {
				using graphics::Light;
				assert(info.type != Light::DIRECTIONAL && index < _culling_info.size());

				const hlsl::LightParameters& params{ _cullable_lights[index] };
				hlsl::LightCullingLightInfo& culling_info{ _culling_info[index] };
				culling_info.range = _bounding_spheres[index].radius = params.range;

				#if USE_BOUNDING_SPHERES
				culling_info.cos_penumbra = -1.f;
				#else
				culling_info.type = params.type;
				#endif

				if (info.type == Light::SPOT) {

					#if USE_BOUNDING_SPHERES
					culling_info.cos_penumbra = params.cos_penumbra;
					#else
					culling_info.cone_radius = calculate_cone_radius(params.range, params.cos_penumbra);
					#endif	
				}

			}

			void swap_cullable_lights(u32 index1, u32 index2) {
				assert(index1 != index2);
				assert(index1 < _cullable_owners.size());
				assert(index2 < _cullable_owners.size());
				assert(index1 < _cullable_lights.size());
				assert(index2 < _cullable_lights.size());
				assert(index2 < _culling_info.size());
				assert(index1 < _culling_info.size());
				assert(index1 < _bounding_spheres.size());
				assert(index2 < _bounding_spheres.size());
				assert(index2 < _cullable_entity_ids.size());
				assert(index1 < _cullable_entity_ids.size());
				assert(id::is_valid(_cullable_owners[index1]) || id::is_valid(_cullable_owners[index2]));

				if (!id::is_valid(_cullable_owners[index2])) {
					std::swap(index1, index2);
				}

				if (!id::is_valid(_cullable_owners[index1])) {
					LightOwner& owner2{ _owners[_cullable_owners[index2]] };
					assert(owner2.data_index == index2);
					owner2.data_index = index1;

					_cullable_lights[index1] = _cullable_lights[index2];
					_culling_info[index1] = _culling_info[index2];
					_bounding_spheres[index1] = _bounding_spheres[index2];
					_cullable_entity_ids[index1] = _cullable_entity_ids[index2];
					std::swap(_cullable_owners[index1], _cullable_owners[index2]);
					make_dirty(index1);
					assert(_owners[_cullable_owners[index1]].entity_id == _cullable_entity_ids[index1]);
					assert(!id::is_valid(_cullable_owners[index2]));
				}
				else {
					LightOwner& owner1{ _owners[_cullable_owners[index1]] };
					LightOwner& owner2{ _owners[_cullable_owners[index2]] };
					assert(owner1.data_index == index1);
					assert(owner2.data_index == index2);
					owner1.data_index = index2;
					owner2.data_index = index1;

					std::swap(_cullable_lights[index1], _cullable_lights[index2]);
					std::swap(_culling_info[index1], _culling_info[index2]);
					std::swap(_bounding_spheres[index1], _bounding_spheres[index2]);
					std::swap(_cullable_entity_ids[index1], _cullable_entity_ids[index2]);
					std::swap(_cullable_owners[index1], _cullable_owners[index2]);

					assert(_owners[_cullable_owners[index1]].entity_id == _cullable_entity_ids[index1]);
					assert(_owners[_cullable_owners[index2]].entity_id == _cullable_entity_ids[index2]);

					make_dirty(index1);
					make_dirty(index2);
				}
			}

			CONSTEXPR void make_dirty(u32 index) {
				assert(index < _dirty_bits.size());
				_something_is_dirty = _dirty_bits[index] = dirty_bits_mask;
			}

			util::free_list<LightOwner> _owners;
			util::vector<hlsl::DirectionalLightParameters> _non_cullable_lights;
			util::vector<light_id> _non_cullable_owners;

			util::vector<hlsl::LightParameters> _cullable_lights;
			util::vector<hlsl::LightCullingLightInfo> _culling_info;
			util::vector<hlsl::Sphere> _bounding_spheres;
			util::vector<game_entity::entity_id> _cullable_entity_ids;
			util::vector<light_id> _cullable_owners;
			util::vector<u8> _dirty_bits;

			util::vector<u8> _transform_flags_cache;
			u32 _enabled_cullable_light_count{0};
			u8 _something_is_dirty{ 0 };

			friend class D3D12LightBuffer;
		};

		class D3D12LightBuffer {
		public:
			D3D12LightBuffer() = default;
			CONSTEXPR void update_light_buffers(LightSet& set, u64 light_set_key, u32 frame_index) {

				const u32 non_cullable_light_count{ set.non_cullable_light_count() };

				if (non_cullable_light_count) {
					const u32 need_size{ non_cullable_light_count * sizeof(hlsl::DirectionalLightParameters) };
					const u32 current_size{ _buffers[LightBuffer::NON_CULLABLE_LIGHT].buffer.size() };

					if (current_size < need_size) {
						resize_buffer(LightBuffer::NON_CULLABLE_LIGHT, need_size, frame_index);
					}

					set.non_cullable_lights((hlsl::DirectionalLightParameters* const)_buffers[LightBuffer::NON_CULLABLE_LIGHT].cpu_address, _buffers[LightBuffer::NON_CULLABLE_LIGHT].buffer.size());
				}
			
				const u32 cullable_light_count{ set.cullable_light_count() };

				if (cullable_light_count) {

					const u32 needed_light_buffer_size{ cullable_light_count * sizeof(hlsl::LightParameters) };
					const u32 needed_culling_buffer_size{ cullable_light_count * sizeof(hlsl::LightCullingLightInfo) };
					const u32 needed_spheres_buffer_size{ cullable_light_count * sizeof(hlsl::Sphere) };
					const u32 current_light_buffer_size{ _buffers[LightBuffer::CULLABLE_LIGHT].buffer.size() };


					bool buffers_resized{ false };
					if (current_light_buffer_size < needed_light_buffer_size) {
						resize_buffer(LightBuffer::CULLABLE_LIGHT, (needed_light_buffer_size * 3) >> 1, frame_index);
						resize_buffer(LightBuffer::CULLING_INFO, (needed_culling_buffer_size * 3) >> 1, frame_index);
						resize_buffer(LightBuffer::BOUNDING_SPHERES, (needed_spheres_buffer_size * 3) >> 1, frame_index);
						buffers_resized = true;
					}

					const u32 index_mask{ 1UL << frame_index };

					if (buffers_resized || _current_light_set_key != light_set_key) {
						memcpy(_buffers[LightBuffer::CULLABLE_LIGHT].cpu_address, set._cullable_lights.data(), needed_light_buffer_size);
						memcpy(_buffers[LightBuffer::CULLING_INFO].cpu_address, set._culling_info.data(), needed_culling_buffer_size);
						memcpy(_buffers[LightBuffer::BOUNDING_SPHERES].cpu_address, set._bounding_spheres.data(), needed_spheres_buffer_size);
						_current_light_set_key = light_set_key;

						for (u32 i{ 0 }; i < cullable_light_count; ++i) {
							set._dirty_bits[i] &= ~index_mask;
						}
					}
					else if (set._something_is_dirty) {
						for (u32 i{ 0 }; i < set.cullable_light_count(); ++i) {
							if (set._dirty_bits[i] & index_mask) {
								assert(i * sizeof(hlsl::LightParameters) < needed_light_buffer_size);
								assert(i * sizeof(hlsl::LightCullingLightInfo) < needed_culling_buffer_size);
								u8* const light_dst{ _buffers[LightBuffer::CULLABLE_LIGHT].cpu_address + (i * sizeof(hlsl::LightParameters)) };
								u8* const culling_dst{ _buffers[LightBuffer::CULLING_INFO].cpu_address + (i * sizeof(hlsl::LightCullingLightInfo)) };
								u8* const bounding_dst{ _buffers[LightBuffer::BOUNDING_SPHERES].cpu_address + (i * sizeof(hlsl::Sphere)) };
								memcpy(light_dst, &set._cullable_lights[i], sizeof(hlsl::LightParameters));
								memcpy(culling_dst, &set._culling_info[i], sizeof(hlsl::LightCullingLightInfo));
								memcpy(bounding_dst, &set._bounding_spheres[i], sizeof(hlsl::Sphere));
								set._dirty_bits[i] &= ~index_mask;
							}
						}
					}

					set._something_is_dirty &= ~index_mask;
					assert(_current_light_set_key == light_set_key);
				}
			}

			constexpr void release() {
				for (u32 i{ 0 }; i < LightBuffer::count; ++i) {
					_buffers[i].buffer.release();
					_buffers[i].cpu_address = nullptr;
				}
			}

			constexpr D3D12_GPU_VIRTUAL_ADDRESS non_cullable_lights() const { return _buffers[LightBuffer::NON_CULLABLE_LIGHT].buffer.gpu_address(); }
			constexpr D3D12_GPU_VIRTUAL_ADDRESS cullable_lights() const { return _buffers[LightBuffer::CULLABLE_LIGHT].buffer.gpu_address(); }
			constexpr D3D12_GPU_VIRTUAL_ADDRESS culling_info() const { return _buffers[LightBuffer::CULLING_INFO].buffer.gpu_address(); }
			constexpr D3D12_GPU_VIRTUAL_ADDRESS bounding_spheres() const { return _buffers[LightBuffer::BOUNDING_SPHERES].buffer.gpu_address(); }

		private:
			struct LightBuffer {
				enum Type : u32 {
					NON_CULLABLE_LIGHT,
					CULLABLE_LIGHT,
					CULLING_INFO,
					BOUNDING_SPHERES,

					count
				};

				D3D12Buffer buffer{};
				u8* cpu_address{ nullptr };
			};

			void resize_buffer(LightBuffer::Type type, u32 size, [[maybe_unused]] u32 frame_index) {
				assert(type < LightBuffer::count);
				if (!size || _buffers[type].buffer.size() >= math::align_size_up<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(size)) return;

				_buffers[type].buffer = D3D12Buffer{ ConstantBuffer::get_default_init_info(size),true };
				NAME_D3D12_OBJECT_INDEXED(_buffers[type].buffer.buffer(), frame_index, type == LightBuffer::NON_CULLABLE_LIGHT ? L"Non-cullable Light Buffer" : type == LightBuffer::CULLABLE_LIGHT ? L"Cullable Light Buffer" : type == LightBuffer::CULLING_INFO ? L"Light Culling Info Buffer" : L"Bounding Spheres Buffer");

				D3D12_RANGE range{};
				DXCall(_buffers[type].buffer.buffer()->Map(0, &range, (void**)(&_buffers[type].cpu_address)));
				assert(_buffers[type].cpu_address);
			}

			LightBuffer _buffers[LightBuffer::count]{};
			u64 _current_light_set_key{ 0 };
		};

		std::unordered_map<u64, LightSet> light_sets;
		D3D12LightBuffer light_buffers[FRAME_BUFFER_COUNT];

		constexpr void set_is_enabled(LightSet& set, light_id id, const void* const data, [[maybe_unused]] u32 size) {
			bool is_enabled{ *(bool*)data };
			assert(sizeof(is_enabled) == size);
			set.enable(id, is_enabled);
		}

		constexpr void set_intensity(LightSet& set, light_id id, const void* const data, [[maybe_unused]] u32 size) {
			f32 intensity{ *(f32*)data };
			assert(sizeof(intensity) == size);
			set.intensity(id, intensity);
		}

		constexpr void set_color(LightSet& set, light_id id, const void* const data, [[maybe_unused]] u32 size) {
			math::v3 color{ *(math::v3*)data };
			assert(sizeof(color) == size);
			set.color(id, color);
		}

		CONSTEXPR void set_attenuation(LightSet& set, light_id id, const void* const data, [[maybe_unused]] u32 size) {
			math::v3 attenuation{ *(math::v3*)data };
			assert(sizeof(attenuation) == size);
			set.attenuation(id, attenuation);
		}

		CONSTEXPR void set_range(LightSet& set, light_id id, const void* const data, [[maybe_unused]] u32 size) {
			f32 range{ *(f32*)data };
			assert(sizeof(range) == size);
			set.range(id, range);
		}

		void set_umbra(LightSet& set, light_id id, const void* const data, [[maybe_unused]] u32 size) {
			f32 umbra{ *(f32*)data };
			assert(sizeof(umbra) == size);
			set.umbra(id, umbra);
		}

		void set_penumbra(LightSet& set, light_id id, const void* const data, [[maybe_unused]] u32 size) {
			f32 penumbra{ *(f32*)data };
			assert(sizeof(penumbra) == size);
			set.penumbra(id, penumbra);
		}

		constexpr void get_is_enabled(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			bool* const is_enabled{ (bool* const)data };
			assert(sizeof(bool) == size);
			*is_enabled = set.is_enabled(id);
		}

		constexpr void get_intensity(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			f32* const intensity{ (f32* const)data };
			assert(sizeof(f32) == size);
			*intensity = set.intensity(id);
		}

		constexpr void get_color(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			math::v3* const color{ (math::v3* const)data };
			assert(sizeof(math::v3) == size);
			*color = set.color(id);
		}

		CONSTEXPR void get_attenuation(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			math::v3* const attenuation{ (math::v3* const)data };
			assert(sizeof(math::v3) == size);
			*attenuation = set.attenuation(id);
		}

		CONSTEXPR void get_range(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			f32* const range{ (f32* const)data };
			assert(sizeof(f32) == size);
			*range = set.range(id);
		}

		void get_umbra(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			f32* const umbra{ (f32* const)data };
			assert(sizeof(f32) == size);
			*umbra = set.umbra(id);
		}

		void get_penumbra(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			f32* const penumbra{ (f32* const)data };
			assert(sizeof(f32) == size);
			*penumbra = set.penumbra(id);
		}

		constexpr void get_type(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			graphics::Light::Type* const type{ (graphics::Light::Type* const)data };
			assert(sizeof(graphics::Light::Type) == size);
			*type = set.type(id);
		}

		constexpr void get_entity_id(const LightSet& set, light_id id, void* const data, [[maybe_unused]] u32 size) {
			id::id_type* const entity_id{ (id::id_type* const)data };
			assert(sizeof(id::id_type) == size);
			*entity_id = set.entity_id(id);
		}

		constexpr void empty_set(LightSet&, light_id, const void* const, u32) {}

		using set_function = void(*)(LightSet&, light_id, const void* const, u32);
		using get_function = void(*)(const LightSet&, light_id, void* const, u32);

		constexpr set_function set_functions[]{
			set_is_enabled,
			set_intensity,
			set_color,
			set_attenuation,
			set_range,
			set_umbra,
			set_penumbra,
			empty_set,
			empty_set
		};

		static_assert(_countof(set_functions) == graphics::LightParameter::count);

		constexpr get_function get_functions[]{
			get_is_enabled,
			get_intensity,
			get_color,
			get_attenuation,
			get_range,
			get_umbra,
			get_penumbra,
			get_type,
			get_entity_id
		};

		static_assert(_countof(get_functions) == graphics::LightParameter::count);

		#undef CONSTEXPR
	}

	bool initialize() { return true; }

	void shutdown() {
		assert(light_sets.empty());

		for (u32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i) {
			light_buffers[i].release();
		}
	}

	void create_light_set(u64 light_set_key) {
		assert(!light_sets.count(light_set_key));
		light_sets[light_set_key] = {};
	}

	void remove_light_set(u64 light_set_key) {
		assert(light_sets.count(light_set_key));
		assert(!light_sets[light_set_key].has_lights());
		light_sets.erase(light_set_key);
	}

	graphics::Light create(LightInitInfo info) {
		assert(light_sets.count(info.light_set_key));
		assert(id::is_valid(info.entity_id));
		return light_sets[info.light_set_key].add(info);
	}

	void remove(light_id id, u64 light_set_key) {
		assert(light_sets.count(light_set_key));
		light_sets[light_set_key].remove(id);
	}

	void set_parameter(light_id id, u64 light_set_key, LightParameter::Parameter parameter, const void* const data, u32 data_size) {
		assert(data && data_size);
		assert(light_sets.count(light_set_key));
		assert(parameter < LightParameter::count && set_functions[parameter] != empty_set);
		set_functions[parameter](light_sets[light_set_key], id, data, data_size);
	}

	void get_parameter(light_id id, u64 light_set_key, LightParameter::Parameter parameter, void* const data, u32 data_size) {
		assert(data && data_size);
		assert(light_sets.count(light_set_key));
		assert(parameter < LightParameter::count);
		get_functions[parameter](light_sets[light_set_key], id, data, data_size);
	}

	void update_light_buffers(const D3D12FrameInfo& info) {
		const u64 light_set_key{ info.info->light_set_key };
		assert(light_sets.count(light_set_key));

		LightSet& set{ light_sets[light_set_key] };

		if (!set.has_lights()) return;

		set.update_transforms();
		const u32 frame_index{ info.frame_index };
		D3D12LightBuffer& light_buffer{ light_buffers[frame_index] };
		light_buffer.update_light_buffers(set, light_set_key, frame_index);
	}

	D3D12_GPU_VIRTUAL_ADDRESS non_cullable_light_buffer(u32 frame_index) {
		const D3D12LightBuffer& light_buffer{ light_buffers[frame_index] };
		return light_buffer.non_cullable_lights();
	}

	D3D12_GPU_VIRTUAL_ADDRESS bounding_spheres_buffer(u32 frame_index) {
		const D3D12LightBuffer& light_buffer{ light_buffers[frame_index] };
		return light_buffer.bounding_spheres();
	}

	D3D12_GPU_VIRTUAL_ADDRESS cullable_light_buffer(u32 frame_index) {
		const D3D12LightBuffer& light_buffer{ light_buffers[frame_index] };
		return light_buffer.cullable_lights();
	}

	D3D12_GPU_VIRTUAL_ADDRESS culling_info_buffer(u32 frame_index) {
		const D3D12LightBuffer& light_buffer{ light_buffers[frame_index] };
		return light_buffer.culling_info();
	}

	u32 non_cullable_light_count(u64 light_set_key) {
		assert(light_sets.count(light_set_key));
		return light_sets[light_set_key].non_cullable_light_count();
	}

	u32 cullable_light_count(u64 light_set_key) {
		assert(light_sets.count(light_set_key));
		return light_sets[light_set_key].cullable_light_count();
	}
}