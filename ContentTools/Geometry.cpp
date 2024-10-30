#include "Geometry.h"
#include "../packages/MikkTSpace/mikktspace.h"
#include "Utilities/IOStream.h"

namespace lightning::tools {
	namespace {

		using namespace math;
		using namespace DirectX;

		s32 mikk_get_num_faces(const SMikkTSpaceContext* context) {
			const Mesh& m{ *(Mesh*)(context->m_pUserData) };
			return (s32)m.indicies.size() / 3;
		}

		s32 mikk_get_num_verticies_of_face([[maybe_unused]] const SMikkTSpaceContext* context, [[maybe_unused]] s32 face_index) { return 3; }

		void mikk_get_position(const SMikkTSpaceContext* context, f32 position[3], s32 face_index, s32 vert_index) {
			const Mesh& m{ *(Mesh*)(context->m_pUserData) };
			const u32 index{ m.indicies[face_index * 3 + vert_index] };
			const math::v3& p{ m.verticies[index].position };
			position[0] = p.x;
			position[1] = p.y;
			position[2] = p.z;
		}

		void mikk_get_normal(const SMikkTSpaceContext* context, f32 normal[3], s32 face_index, s32 vert_index) {
			const Mesh& m{ *(Mesh*)(context->m_pUserData) };
			const u32 index{ m.indicies[face_index * 3 + vert_index] };
			const math::v3& n{ m.verticies[index].normal };
			normal[0] = n.x;
			normal[1] = n.y;
			normal[2] = n.z;
		}

		void mikk_get_tex_coord(const SMikkTSpaceContext* context, f32 texture[2], s32 face_index, s32 vert_index) {
			const Mesh& m{ *(Mesh*)(context->m_pUserData) };
			const u32 index{ m.indicies[face_index * 3 + vert_index] };
			const math::v2& uv{ m.verticies[index].uv };
			texture[0] = uv.x;
			texture[1] = uv.y;
		}

		void mikk_set_tspace_basic(const SMikkTSpaceContext* context, const f32 tangent[3], f32 sign, s32 face_index, s32 vert_index) {
			Mesh& m{ *(Mesh*)(context->m_pUserData) };
			const u32 index{ m.indicies[face_index * 3 + vert_index] };
			math::v4& t{ m.verticies[index].tangent };
			t.x = tangent[0];
			t.y = tangent[1];
			t.z = tangent[2];
			t.w = sign;
		}

		void calculate_mikk_tspace(Mesh& m) {
			m.tangents.clear();

			SMikkTSpaceInterface mikk_interface{};
			mikk_interface.m_getNumFaces = mikk_get_num_faces;
			mikk_interface.m_getNumVerticesOfFace = mikk_get_num_verticies_of_face;
			mikk_interface.m_getPosition = mikk_get_position;
			mikk_interface.m_getNormal = mikk_get_normal;
			mikk_interface.m_getTexCoord = mikk_get_tex_coord;
			mikk_interface.m_setTSpaceBasic = mikk_set_tspace_basic;
			mikk_interface.m_setTSpace = nullptr;

			SMikkTSpaceContext mikk_context{};
			mikk_context.m_pInterface = &mikk_interface;
			mikk_context.m_pUserData = (void*)&m;

			genTangSpaceDefault(&mikk_context);
		}

		void calculate_tangents(Mesh& m) {
			m.tangents.clear();

			const u32 num_indicies{ (u32)m.raw_indicies.size() };
			util::vector<XMVECTOR> tangents(num_indicies, XMVectorZero());
			util::vector<XMVECTOR> bitangents(num_indicies, XMVectorZero());
			util::vector<XMVECTOR> positions(num_indicies);

			for (u32 i{ 0 }; i < num_indicies; ++i) {
				positions[i] = XMLoadFloat3(&m.verticies[m.indicies[i]].position);
			}

			for (u32 i{ 0 }; i < num_indicies; i += 3) {
				const u32 i0{ i };
				const u32 i1{ i + 1 };
				const u32 i2{ i + 2 };

				const XMVECTOR& p0{ positions[i0] };
				const XMVECTOR& p1{ positions[i1] };
				const XMVECTOR& p2{ positions[i2] };

				const math::v2& uv0{ m.verticies[m.indicies[i0]].uv };
				const math::v2& uv1{ m.verticies[m.indicies[i1]].uv };
				const math::v2& uv2{ m.verticies[m.indicies[i2]].uv };

				const math::v2 duv1{ uv1.x - uv0.x, uv1.y - uv0.y };
				const math::v2 duv2{ uv2.x - uv0.x, uv2.y - uv0.y };

				const XMVECTOR dp1{ p1 - p0 };
				const XMVECTOR dp2{ p2 - p0 };

				f32 det{ duv1.x * duv2.y - duv1.y * duv2.x };

				if (abs(det) < math::EPSILON) det = math::EPSILON;

				const f32 inv_det{ 1.f / det };
				const XMVECTOR t{ (dp1 * duv2.y - dp2 * duv1.y) * inv_det };
				const XMVECTOR b{ (dp2 * duv1.x - dp1 * duv2.x) * inv_det };

				tangents[i0] += t;
				tangents[i1] += t;
				tangents[i2] += t;
				bitangents[i0] += b;
				bitangents[i1] += b;
				bitangents[i2] += b;
			}

			for (u32 i{ 0 }; i < num_indicies; ++i) {
				const XMVECTOR& t{ tangents[i] };
				const XMVECTOR& b{ bitangents[i] };
				const XMVECTOR& n{ XMLoadFloat3(&m.verticies[m.indicies[i]].normal) };

				math::v3 tangent;
				XMStoreFloat3(&tangent, XMVector3Normalize(t - n * XMVector3Dot(n, t)));
				f32 handedness;
				XMStoreFloat(&handedness, XMVector3Dot(XMVector3Cross(t, b), n));

				handedness = handedness > 0.f ? 1.f : -1.f;

				m.verticies[m.indicies[i]].tangent = { tangent.x, tangent.y, tangent.z, handedness };
			}
		}

		void recalculate_normals(Mesh& m) {
			const u32 num_indicies{ (u32)m.raw_indicies.size() };
			m.normals.reserve(num_indicies);

			for (u32 i{ 0 }; i < num_indicies; ++i) {
				const u32 i0{ m.raw_indicies[i] };
				const u32 i1{ m.raw_indicies[++i] };
				const u32 i2{ m.raw_indicies[++i] };

				XMVECTOR v0{ XMLoadFloat3(&m.positions[i0]) };
				XMVECTOR v1{ XMLoadFloat3(&m.positions[i1]) };
				XMVECTOR v2{ XMLoadFloat3(&m.positions[i2]) };

				XMVECTOR e0{ v1 - v0 };
				XMVECTOR e1{ v2 - v0 };

				XMVECTOR n{ XMVector3Normalize(XMVector3Cross(e0, e1)) };

				XMStoreFloat3(&m.normals[i], n);
				m.normals[i - 1] = m.normals[i];
				m.normals[i - 2] = m.normals[i];
			}
		}

		void process_normals(Mesh& m, f32 smoothing_angle) {
			const f32 cos_alpha{ XMScalarCos(PI - smoothing_angle * PI / 180.f) };
			const bool is_hard_edge{ XMScalarNearEqual(smoothing_angle, 180.f, EPSILON) };
			const bool is_soft_edge{ XMScalarNearEqual(smoothing_angle, 0.f, EPSILON) };
			const u32 num_indicies{ (u32)m.raw_indicies.size() };
			const u32 num_verticies{ (u32)m.positions.size() };
			assert(num_indicies && num_verticies);

			m.indicies.resize(num_indicies);

			util::vector<util::vector<u32>> idx_ref(num_verticies);
			for (u32 i{ 0 }; i < num_indicies; ++i) {
				idx_ref[m.raw_indicies[i]].emplace_back(i);
			}

			for (u32 i{ 0 }; i < num_verticies; ++i) {
				util::vector<u32>& refs{ idx_ref[i] };
				u32 num_refs{ (u32)refs.size() };

				for (u32 j{ 0 }; j < num_refs; ++j) {
					m.indicies[refs[j]] = (u32)m.verticies.size();
					Vertex& v{ m.verticies.emplace_back() };
					v.position = m.positions[m.raw_indicies[refs[j]]];

					XMVECTOR n1{ XMLoadFloat3(&m.normals[refs[j]]) };
					if (!is_hard_edge) {
						for (u32 k{ j + 1 }; k < num_refs; ++k) {
							f32 cos_theta{ 0.f };
							XMVECTOR n2{ XMLoadFloat3(&m.normals[refs[k]]) };
							if (!is_soft_edge) {
								// cos(angle) = dot(n1, n2) / (|n1| * |n2|)
								XMStoreFloat(&cos_theta, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
							}

							if (is_soft_edge || cos_theta >= cos_alpha) {
								n1 += n2;
								m.indicies[refs[k]] = m.indicies[refs[j]];
								refs.erease(refs.begin() + k);
								--num_refs;
								--k;
							}
						}
					}
					XMStoreFloat3(&v.normal, XMVector3Normalize(n1));
				}
			}
		}

		void process_uvs(Mesh& m) {
			util::vector<Vertex> old_verticies;
			old_verticies.swap(m.verticies);
			util::vector<u32> old_indicies(m.indicies.size());
			old_indicies.swap(m.indicies);

			const u32 num_verticies{ (u32)old_verticies.size() };
			const u32 num_indicies{ (u32)old_indicies.size() };

			assert(num_verticies && num_indicies);

			util::vector<util::vector<u32>> idx_ref(num_verticies);
			for (u32 i{ 0 }; i < num_indicies; ++i) {
				idx_ref[old_indicies[i]].emplace_back(i);
			}
			for (u32 i{ 0 }; i < num_indicies; ++i) {
				util::vector<u32>& refs{ idx_ref[i] };
				u32 num_refs{ (u32)refs.size() };
				for (u32 j{ 0 }; j < num_refs; ++j) {
					m.indicies[refs[j]] = (u32)m.verticies.size();
					Vertex& v{ old_verticies[old_indicies[refs[j]]] };
					v.uv = m.uv_sets[0][refs[j]];
					m.verticies.emplace_back(v);

					for (u32 k{ j + 1 }; k < num_refs; ++k) {
						v2& uv1{ m.uv_sets[0][refs[k]] };
						if (XMScalarNearEqual(v.uv.x, uv1.x, EPSILON) && XMScalarNearEqual(v.uv.y, uv1.y, EPSILON)) {
							m.indicies[refs[k]] = m.indicies[refs[j]];
							refs.erease(refs.begin() + k);
							--num_refs;
							--k;
						}
					}
				}
			}
		}

		void process_tangents(Mesh& m) {
			if (m.tangents.size() != m.raw_indicies.size()) { return; }

			util::vector<Vertex> old_vertices;
			old_vertices.swap(m.verticies);
			util::vector<u32> old_indices(m.indicies.size());
			old_indices.swap(m.indicies);

			const u32 num_vertices{ (u32)old_vertices.size() };
			const u32 num_indices{ (u32)old_indices.size() };
			assert(num_vertices && num_indices);

			util::vector<util::vector<u32>> idx_ref{ num_vertices };

			for (u32 i{ 0 }; i < num_indices; ++i) {
				idx_ref[old_indices[i]].emplace_back(i);
			}

			for (u32 i{ 0 }; i < num_vertices; ++i) {
				util::vector<u32>& refs{ idx_ref[i] };
				u32 num_refs{ (u32)refs.size() };

				for (u32 j{ 0 }; j < num_refs; ++j) {
					const v4& tj{ m.tangents[refs[j]] };
					Vertex& v{ old_vertices[old_indices[refs[j]]] };
					v.tangent = tj;
					m.indicies[refs[j]] = (u32)m.verticies.size();
					m.verticies.emplace_back(v);

					XMVECTOR xm_tj{ XMLoadFloat4(&tj) };
					XMVECTOR xm_epsilon(XMVectorReplicate(EPSILON));

					for (u32 k{ j + 1 }; k < num_refs; ++k) {
						XMVECTOR xm_tangent{ XMLoadFloat4(&m.tangents[refs[k]]) };

						if (XMVector4NearEqual(xm_tj, xm_tangent, xm_epsilon)) {
							m.indicies[refs[k]] = m.indicies[refs[j]];
							refs.erease(refs.begin() + k);
							--num_refs;
							--k;
						}
					}
				}
			}
		}

		u64 get_vertex_elements_size(elements::ElementsType::Type elements_type) {
			using namespace elements;

			switch (elements_type) {
				case ElementsType::STATIC_NORMAL: return sizeof(StaticNormal);
				case ElementsType::STATIC_NORMAL_TEXTURE: return sizeof(StaticNormalTexture);
				case ElementsType::STATIC_COLOR: return sizeof(StaticColor);
				case ElementsType::SKELETAL: return sizeof(Skeletal);
				case ElementsType::SKELETAL_COLOR: return sizeof(SkeletalColor);
				case ElementsType::SKELETAL_NORMAL: return sizeof(SkeletalNormal);
				case ElementsType::SKELETAL_NORMAL_COLOR: return sizeof(SkeletalNormalColor);
				case ElementsType::SKELETAL_NORMAL_TEXTURE: return sizeof(SkeletalNormalTexture);
				case ElementsType::SKELETAL_NORMAL_TEXTURE_COLOR: return sizeof(SkeletalNormalTextureColor);
				default: return 0;
			}
		}

		void pack_verticies(Mesh& m) {
			const u32 num_verticies{ (u32)m.verticies.size() };
			assert(num_verticies);

			m.position_buffer.resize(sizeof(math::v3) * num_verticies);
			math::v3* const position_buffer{ (math::v3* const)m.position_buffer.data() };

			for (u32 i{ 0 }; i < num_verticies; ++i) {
				position_buffer[i] = m.verticies[i].position;
			}

			struct u16v2 { u16 x, y; };
			struct u8v3 { u8 x, y, z; };

			util::vector<u8> t_signs{ num_verticies };
			util::vector<u16v2> normals{ num_verticies };
			util::vector<u16v2> tangents{ num_verticies };
			util::vector<u8v3> joint_weights{ num_verticies };

			if (m.elements_type & elements::ElementsType::STATIC_NORMAL) {
				for (u32 i{ 0 }; i < num_verticies; ++i) {
					Vertex& v{ m.verticies[i] };
					t_signs[i] = (u8)((v.normal.z > 0.f) << 2);
					normals[i] = {
						(u16)pack_float<16>(v.normal.x, -1.f, 1.f),
						(u16)pack_float<16>(v.normal.y, -1.f, 1.f)
					};
				}

				if (m.elements_type & elements::ElementsType::STATIC_NORMAL_TEXTURE) {
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						t_signs[i] |= (u8)((v.tangent.w > 0.f) | ((v.tangent.z > 0.f) << 1));
						tangents[i] = {
							(u16)pack_float<16>(v.tangent.x, -1.f, 1.f),
							(u16)pack_float<16>(v.tangent.y, -1.f, 1.f)
						};
					}
				}
			}

			if (m.elements_type & elements::ElementsType::SKELETAL) {
				for (u32 i{ 0 }; i < num_verticies; ++i) {
					Vertex& v{ m.verticies[i] };
					joint_weights[i] = {
						(u8)pack_unit_float<8>(v.joint_weights.x),
						(u8)pack_unit_float<8>(v.joint_weights.y),
						(u8)pack_unit_float<8>(v.joint_weights.z)
					};
				}
			}

			m.element_buffer.resize(get_vertex_elements_size(m.elements_type)* num_verticies);

			using namespace elements;

			switch (m.elements_type) {
				case ElementsType::STATIC_COLOR: {
					StaticColor* const element_buffer{ (StaticColor* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						element_buffer[i] = { { v.red, v.green, v.blue }, {} };
					}
				}
				break;

				case ElementsType::STATIC_NORMAL: {
					StaticNormal* const element_buffer{ (StaticNormal* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						element_buffer[i] = { { v.red, v.green, v.blue }, t_signs[i], { normals[i].x, normals[i].y } };
					}
				}
				break;

				case ElementsType::STATIC_NORMAL_TEXTURE: {
					StaticNormalTexture* const element_buffer{ (StaticNormalTexture* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						element_buffer[i] = {
							{ v.red, v.green, v.blue },
							t_signs[i],
							{ normals[i].x, normals[i].y },
							{ tangents[i].x, tangents[i].y },
							v.uv
						};
					}
				}
				break;

				case ElementsType::SKELETAL: {
					Skeletal* const element_buffer{ (Skeletal* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						const u16 indicies[4]{ (u16)v.joint_indicies.x, (u16)v.joint_indicies.y, (u16)v.joint_indicies.z, (u16)v.joint_indicies.w };
						element_buffer[i] = { 
							{ joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
							{},
							{ indicies[0], indicies[1], indicies[2], indicies[3] }
						};
					}
				}
				break;

				case ElementsType::SKELETAL_COLOR: {
					SkeletalColor* const element_buffer{ (SkeletalColor* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						const u16 indicies[4]{ (u16)v.joint_indicies.x, (u16)v.joint_indicies.y, (u16)v.joint_indicies.z, (u16)v.joint_indicies.w };
						element_buffer[i] = {
							{ joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
							{},
							{ indicies[0], indicies[1], indicies[2], indicies[3] },
							{ v.red, v.green, v.blue },
							{}
						};
					}
				}
				break;

				case ElementsType::SKELETAL_NORMAL: {
					SkeletalNormal* const element_buffer{ (SkeletalNormal* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						const u16 indicies[4]{ (u16)v.joint_indicies.x, (u16)v.joint_indicies.y, (u16)v.joint_indicies.z, (u16)v.joint_indicies.w };
						element_buffer[i] = {
							{ joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
							t_signs[i],
							{ indicies[0], indicies[1], indicies[2], indicies[3] },
							{ normals[i].x, normals[i].y }
						};
					}
				}
				break;

				case ElementsType::SKELETAL_NORMAL_COLOR: {
					SkeletalNormalColor* const element_buffer{ (SkeletalNormalColor* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						const u16 indicies[4]{ (u16)v.joint_indicies.x, (u16)v.joint_indicies.y, (u16)v.joint_indicies.z, (u16)v.joint_indicies.w };
						element_buffer[i] = {
							{ joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
							t_signs[i],
							{ indicies[0], indicies[1], indicies[2], indicies[3] },
							{ normals[i].x, normals[i].y },
							{ v.red, v.green, v.blue },
							{}
						};
					}
				}
				break;

				case ElementsType::SKELETAL_NORMAL_TEXTURE: {
					SkeletalNormalTexture* const element_buffer{ (SkeletalNormalTexture* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						const u16 indicies[4]{ (u16)v.joint_indicies.x, (u16)v.joint_indicies.y, (u16)v.joint_indicies.z, (u16)v.joint_indicies.w };
						element_buffer[i] = {
							{ joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
							t_signs[i],
							{ indicies[0], indicies[1], indicies[2], indicies[3] },
							{ normals[i].x, normals[i].y },
							{ tangents[i].x, tangents[i].y },
							v.uv
						};
					}
				}
				break;

				case ElementsType::SKELETAL_NORMAL_TEXTURE_COLOR: {
					SkeletalNormalTextureColor* const element_buffer{ (SkeletalNormalTextureColor* const)m.element_buffer.data() };
					for (u32 i{ 0 }; i < num_verticies; ++i) {
						Vertex& v{ m.verticies[i] };
						const u16 indicies[4]{ (u16)v.joint_indicies.x, (u16)v.joint_indicies.y, (u16)v.joint_indicies.z, (u16)v.joint_indicies.w };
						element_buffer[i] = {
							{ joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
							t_signs[i],
							{ indicies[0], indicies[1], indicies[2], indicies[3] },
							{ normals[i].x, normals[i].y },
							{ tangents[i].x, tangents[i].y },
							v.uv,
							{ v.red, v.green, v.blue },
							{}
						};
					}
				}
				break;
			}
		}

		elements::ElementsType::Type determine_elements_type(const Mesh& m) {
			using namespace elements;

			ElementsType::Type type{};

			if (m.normals.size()) {
				if (m.uv_sets.size() && m.uv_sets[0].size()) {
					type = ElementsType::STATIC_NORMAL_TEXTURE;
				}
				else {
					type = ElementsType::STATIC_NORMAL;
				}
			}
			else if (m.colors.size()) {
				type = ElementsType::STATIC_COLOR;
			}

			// TODO: Expand to Skeletal Meshes

			return type;
		}

		void process_verticies(Mesh& m, const GeometryImportSettings& settings) {
			assert((m.raw_indicies.size() % 3) == 0);
			if (settings.calculate_normals || m.normals.empty()) {
				recalculate_normals(m);
			}
			process_normals(m, settings.smoothing_angle);

			if (!m.uv_sets.empty()) {
				process_uvs(m);
			}

			if ((settings.calculate_tangents || m.tangents.empty()) && !m.uv_sets.empty()) {
				calculate_mikk_tspace(m);
				//calculate_tangents(m);
			}

			if (!m.tangents.empty()) {
				process_tangents(m);
			}

			m.elements_type = determine_elements_type(m);
			pack_verticies(m);
		}
		u64 get_mesh_size(const Mesh& m) {
			const u64 num_verticies{ m.verticies.size() };
			const u64 position_buffer_size{ m.position_buffer.size() };
			assert(position_buffer_size == sizeof(math::v3) * num_verticies);
			const u64 element_buffer_size{ m.element_buffer.size() };
			assert(element_buffer_size == get_vertex_elements_size(m.elements_type) * num_verticies);
			const u64 index_size{ (num_verticies < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
			const u64 index_buffer_size{ index_size * m.indicies.size() };
			constexpr u64 su32{ sizeof(u32) };
			const u64 size{
				su32 +					// name length
				m.name.size() +			// mesh name string size
				su32 +					// lod id
				su32 +					// vertex element size
				su32 +					// element type enum
				su32 +					// number of verticies
				su32 +					// index size (16 bit || 32 bit)
				su32 +					// number of indicies
				sizeof(f32) +			// LOD threshold
				position_buffer_size +	// room for vertex positions
				element_buffer_size +	// room for vertex elements
				index_buffer_size		// room for indicies
			};
			return size;
		}

		u64 get_scene_size(const Scene& scene) {
			constexpr u64 su32{ sizeof(u32) };
			u64 size{
				su32 +					// scene name length
				scene.name.size() +		// scene name string size
				su32					// number of LODs
			};

			for (const auto& lod : scene.lod_groups) {
				u64 lod_size{
					su32 +				// LOD name length
					lod.name.size() +	// LOD name string size
					su32				// number of mashes in this LOD
				};

				for (const auto& m : lod.meshes) {
					lod_size += get_mesh_size(m);
				}
				size += lod_size;
			}
			return size;
		}

		void pack_mesh_data(const Mesh& m, util::BlobStreamWriter& blob) {

			blob.write((u32)m.name.size());
			blob.write(m.name.c_str(), m.name.size());
			blob.write(m.lod_id);

			const u32 elements_size{ (u32)get_vertex_elements_size(m.elements_type) };
			blob.write(elements_size);
			blob.write((u32)m.elements_type);

			const u32 num_verticies{ (u32)m.verticies.size() };
			blob.write(num_verticies);
			
			const u32 index_size{ (num_verticies < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
			blob.write(index_size);
			
			const u32 num_indicies{ (u32)m.indicies.size() };
			blob.write(num_indicies);

			blob.write(m.lod_threshold);

			assert(m.position_buffer.size() == sizeof(math::v3) * num_verticies);
			blob.write(m.position_buffer.data(), m.position_buffer.size());

			assert(m.element_buffer.size() == elements_size * num_verticies);
			blob.write(m.element_buffer.data(), m.element_buffer.size());

			const u32 index_buffer_size{ index_size * num_indicies };
			const u8* data{ (const u8*)m.indicies.data() };
			util::vector<u16> indicies;
			if (index_size == sizeof(u16)) {
				indicies.resize(num_indicies);
				for (u32 i{ 0 }; i < num_indicies; ++i) indicies[i] = (u16)m.indicies[i];
				data = (const u8*)indicies.data();
			}
			blob.write(data, index_buffer_size);
		}

		bool split_meshes_by_material(u32 material_idx, const Mesh& m, Mesh& submesh) {
			submesh.name = m.name;
			submesh.lod_threshold = m.lod_threshold;
			submesh.lod_id = m.lod_id;
			submesh.material_used.emplace_back(material_idx);
			submesh.uv_sets.resize(m.uv_sets.size());

			const u32 num_polys{ (u32)m.raw_indicies.size() / 3 };
			util::vector<u32> vertex_ref(m.positions.size(), u32_invalid_id);

			for (u32 i{ 0 }; i < num_polys; ++i) {
				const u32 mtl_idx{ m.material_indicies[i] };
				if (mtl_idx != material_idx) continue;

				const u32 index{ i * 3 };

				for (u32 j = index; j < index + 3; ++j) {
					const u32 v_idx{ m.raw_indicies[j] };

					if (vertex_ref[v_idx] != u32_invalid_id) {
						submesh.raw_indicies.emplace_back(vertex_ref[v_idx]);
					}
					else {
						submesh.raw_indicies.emplace_back((u32)submesh.positions.size());
						vertex_ref[v_idx] = submesh.raw_indicies.back();
						submesh.positions.emplace_back(m.positions[v_idx]);
					}

					if (m.normals.size()) submesh.normals.emplace_back(m.normals[j]);
					if (m.tangents.size()) submesh.tangents.emplace_back(m.tangents[j]);

					for (u32 k{ 0 }; i < m.uv_sets.size(); ++k) {
						if (m.uv_sets[k].size()) {
							submesh.uv_sets[k].emplace_back(m.uv_sets[k][j]);
						}
					}
				}
			}

			assert((submesh.raw_indicies.size() % 3) == 0);

			return !submesh.raw_indicies.empty();
		}

		void split_meshes_by_material(Scene& scene, Progression* const progression) {
			assert(progression);
			progression->callback(0, 0);

			for (auto& lod : scene.lod_groups) {
				util::vector<Mesh> new_meshes;

				for (const auto& m : lod.meshes) {
					const u32 num_materials{ (u32)m.material_used.size() };
					if (num_materials > 1) {
						for (u32 i{ 0 }; i < num_materials; ++i) {
							Mesh submesh{};
							if (split_meshes_by_material(m.material_used[i], m, submesh)) {
								new_meshes.emplace_back(submesh);
							}
						}
					}
					else {
						new_meshes.emplace_back(m);
					}
				}
				progression->callback(progression->value(), progression->max_value() + (u32)new_meshes.size());
				new_meshes.swap(lod.meshes);
			}
		}

		template <typename T> void append_to_vector_pod(util::vector<T>& dst, const util::vector<T>& src) {
			if (src.empty()) return;

			const u32 num_elements{ (u32)dst.size() };
			dst.resize(dst.size() + src.size());
			memcpy(&dst[num_elements], src.data(), src.size() * sizeof(T));
		}
	}

	void process_scene(Scene& scene, const GeometryImportSettings& settings, Progression* const progression) {
		assert(progression);
		split_meshes_by_material(scene, progression);
		
		for (auto& lod : scene.lod_groups) {
			for (auto& m : lod.meshes) {
				process_verticies(m, settings);
				progression->callback(progression->value() + 1, progression->max_value());
			}
		}
	}

	void pack_data(const Scene& scene, SceneData& data) {
		const u64 scene_size{ get_scene_size(scene) };
		data.buffer_size = (u32)scene_size;
		data.buffer = (u8*)CoTaskMemAlloc(scene_size);
		assert(data.buffer);

		util::BlobStreamWriter blob{ data.buffer, data.buffer_size };

		blob.write((u32)scene.name.size());
		blob.write(scene.name.c_str(), scene.name.size());

		blob.write((u32)scene.lod_groups.size());

		for (const auto& lod : scene.lod_groups) {
			blob.write((u32)lod.name.size());
			blob.write(lod.name.c_str(), lod.name.size());

			blob.write((u32)lod.meshes.size());

			for (const auto& m : lod.meshes) {
				pack_mesh_data(m, blob);
			}
		}
		assert(scene_size == blob.offset());
	}

	bool coalesce_meshes(const LodGroup& lod, Mesh& combined_mesh, Progression* const progression) {
		assert(lod.meshes.size());
		const Mesh& first_mesh{ lod.meshes[0] };
		combined_mesh.name = first_mesh.name;
		combined_mesh.elements_type = determine_elements_type(first_mesh);
		combined_mesh.lod_threshold = first_mesh.lod_threshold;
		combined_mesh.lod_id = first_mesh.lod_id;
		combined_mesh.uv_sets.resize(first_mesh.uv_sets.size());

		for (u32 mesh_idx{ 0 }; mesh_idx < lod.meshes.size(); ++mesh_idx) {
			const Mesh& m{ lod.meshes[mesh_idx] };

			if (combined_mesh.elements_type != determine_elements_type(m) || combined_mesh.uv_sets.size() != m.uv_sets.size() || combined_mesh.lod_id != m.lod_id || !math::is_equal(combined_mesh.lod_threshold, m.lod_threshold)) {
				combined_mesh = {};
				return false;
			}
		}

		for (u32 mesh_idx{ 0 }; mesh_idx < lod.meshes.size(); ++mesh_idx) {
			const Mesh& m{ lod.meshes[mesh_idx] };
			const u32 position_count{ (u32)combined_mesh.positions.size() };
			const u32 raw_index_base{ (u32)combined_mesh.raw_indicies.size() };

			append_to_vector_pod(combined_mesh.positions, m.positions);
			append_to_vector_pod(combined_mesh.normals, m.normals);
			append_to_vector_pod(combined_mesh.tangents, m.tangents);
			append_to_vector_pod(combined_mesh.colors, m.colors);

			for (u32 i{ 0 }; i < combined_mesh.uv_sets.size(); ++i) {
				append_to_vector_pod(combined_mesh.uv_sets[i], m.uv_sets[i]);
			}

			append_to_vector_pod(combined_mesh.material_indicies, m.material_indicies);
			append_to_vector_pod(combined_mesh.raw_indicies, m.raw_indicies);

			for (u32 i{ raw_index_base }; i < combined_mesh.raw_indicies.size(); ++i) {
				combined_mesh.raw_indicies[i] += position_count;
			}

			progression->callback(progression->value(), progression->max_value() > 1 ? progression->max_value() - 1 : 1);
		}

		for (const u32 mtl_id : combined_mesh.material_indicies) {
			if (std::find(combined_mesh.material_used.begin(), combined_mesh.material_used.end(), mtl_id) == combined_mesh.material_used.end()) {
				combined_mesh.material_used.emplace_back(mtl_id);
			}
		}

		return true;
	}
}