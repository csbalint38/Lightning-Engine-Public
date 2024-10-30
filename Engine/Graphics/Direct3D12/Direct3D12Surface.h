#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12 {
	class D3D12Surface {
		public:
			constexpr static u32 buffer_count{ 3 };
			constexpr static DXGI_FORMAT default_back_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };

			explicit D3D12Surface(platform::Window window) : _window{ window } {
				assert(window.handle());
			}

			#if USE_STL_VECTOR
			DISABLE_COPY(D3D12Surface);
			constexpr D3D12Surface(D3D12Surface&& o) : _swap_chain{ o._swap_chain }, _window{ o._window }, _current_bb_index{ o._current_bb_index }, _viewport{ o._viewport }, _scissor_rect{ o._scissor_rect }, _allow_tearing{ o._allow_tearing }, _present_flags{ o._present_flags }, _light_culling_id{ o._light_culling_id } {
				for (u32 i{ 0 }; i < buffer_count; ++i) {
					_render_target_data[i] = o._render_target_data[i];
				}
				o.reset();
				}

			constexpr D3D12Surface& operator=(D3D12Surface&& o) {
				assert(this != &o);
				if (this != &o) {
					release();
					move(o);
				}
				return *this;					
			}
			#else
			DISABLE_COPY_AND_MOVE(D3D12Surface)
			#endif

			~D3D12Surface() { release(); }

			void create_swap_chain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue);
			void present() const;
			void resize();
			[[nodiscard]] constexpr u32 width() const { return (u32)_viewport.Width; }
			[[nodiscard]] constexpr u32 height() const { return (u32)_viewport.Height; }
			[[nodiscard]] constexpr ID3D12Resource* const back_buffer() const { return _render_target_data[_current_bb_index].resource; }
			[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv() const { return _render_target_data[_current_bb_index].rtv.cpu; }
			[[nodiscard]] constexpr const D3D12_VIEWPORT& viewport() const { return _viewport; }
			[[nodiscard]] constexpr const D3D12_RECT& scissor_rect() const { return _scissor_rect; }
			[[nodiscard]] constexpr id::id_type light_culling_id() const { return _light_culling_id; }

		private:
			struct RenderTargetData {
				ID3D12Resource* resource{ nullptr };
				DescriptorHandle rtv{};
			};

			#if USE_STL_VECTOR
			constexpr void move(D3D12Surface& o) {
				_swap_chain = o._swap_chain;
				for (u32 i{ 0 }; i < buffer_count; ++i) _render_target_data[i] = o._render_target_data[i];
				_window = o._window;
				_current_bb_index = o._current_bb_index;
				_allow_tearing = o._allow_tearing;
				_present_flags = o._present_flags;
				_viewport = o._viewport;
				_scissor_rect = o._scissor_rect;
				_light_culling_id = o._light_culling_id;

				o.reset();
			}

			constexpr void reset() {
				_swap_chain = nullptr;
				for (u32 i{ 0 }; i < buffer_count; ++i) _render_target_data[i] = {};
				_window = {};
				_current_bb_index = 0;
				_allow_tearing = 0;
				_present_flags = 0;
				_viewport = {};
				_scissor_rect = {};
				_light_culling_id = id::invalid_id;
			}
			#endif

			void finalize();
			void release();

			IDXGISwapChain4* _swap_chain{ nullptr };
			RenderTargetData _render_target_data[buffer_count]{};
			platform::Window _window{};
			mutable u32 _current_bb_index{ 0 };
			u32 _allow_tearing{ 0 };
			u32 _present_flags{ 0 };
			D3D12_VIEWPORT _viewport{};
			D3D12_RECT _scissor_rect{};
			id::id_type _light_culling_id{ id::invalid_id };
	};
}