#pragma once
#include "Direct3D12CommonHeaders.h"

namespace lightning::graphics::direct3d12 {

	class DescriptorHeap;

	struct DescriptorHandle {
		D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
		u32 index{ u32_invalid_id };

		constexpr bool is_valid() const { return cpu.ptr != 0; }
		constexpr bool is_shader_visible() const { return gpu.ptr != 0; }

		#ifdef _DEBUG
		private:
			friend class DescriptorHeap;
			DescriptorHeap* container{ nullptr };
		#endif
	};

	class DescriptorHeap {
		public:
			explicit DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : _type{ type } {};
			DISABLE_COPY_AND_MOVE(DescriptorHeap);
			~DescriptorHeap() { assert(!_heap); }

			bool initialize(u32 capacity, bool is_shader_visible);
			void release();
			void process_deferred_free(u32 frame_ids);

			[[nodiscard]] DescriptorHandle allocate();
			void free(DescriptorHandle& handle);

			[[nodiscard]] constexpr D3D12_DESCRIPTOR_HEAP_TYPE type() const { return _type; };
			[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpu_start() const { return _cpu_start; };
			[[nodiscard]] constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpu_start() const { return _gpu_start; };
			[[nodiscard]] constexpr ID3D12DescriptorHeap* heap() const { return _heap; };
			[[nodiscard]] constexpr u32 capacity() const { return _capacity; }
			[[nodiscard]] constexpr u32 size() const { return _size; }
			[[nodiscard]] constexpr u32 descriptor_size() const { return _descriptor_size; }
			[[nodiscard]] constexpr bool is_shader_visible() const { return _gpu_start.ptr != 0; }

		private:
			ID3D12DescriptorHeap* _heap{ nullptr };
			D3D12_CPU_DESCRIPTOR_HANDLE _cpu_start{};
			D3D12_GPU_DESCRIPTOR_HANDLE _gpu_start{};
			std::unique_ptr<u32[]> _free_handles{};
			util::vector<u32> _deferred_free_indicies[FRAME_BUFFER_COUNT]{};
			std::mutex _mutex{};
			u32 _capacity{ 0 };
			u32 _size{ 0 };
			u32 _descriptor_size{};
			const D3D12_DESCRIPTOR_HEAP_TYPE _type{};
	};

	struct D3D12BufferInitInfo {
		ID3D12Heap1* heap{ nullptr };
		const void* data{ nullptr };
		D3D12_RESOURCE_ALLOCATION_INFO1 allocation_info{};
		D3D12_RESOURCE_STATES initial_state{ D3D12_RESOURCE_STATE_COMMON };
		D3D12_RESOURCE_FLAGS flags{ D3D12_RESOURCE_FLAG_NONE };
		u32 size{ 0 };
		u32 alignment{ 0 };
	};

	class D3D12Buffer {
		public:
			D3D12Buffer() = default;
			explicit D3D12Buffer(const D3D12BufferInitInfo& info, bool is_cpu_accessible);
			DISABLE_COPY(D3D12Buffer);
			constexpr D3D12Buffer(D3D12Buffer&& o) : _buffer{ o._buffer }, _gpu_address{ o._gpu_address }, _size{ o._size } { o.reset(); }

			constexpr D3D12Buffer& operator=(D3D12Buffer&& o) {
				assert(this != &o);
				if (this != &o) {
					release();
					move(o);
				}

				return *this;
			}

			void release();

			[[nodiscard]] constexpr ID3D12Resource* const buffer() const { return _buffer; }
			[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return _gpu_address; }
			[[nodiscard]] constexpr u32 size() const { return _size; }

			~D3D12Buffer() { release(); }

		private:
			ID3D12Resource* _buffer{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS _gpu_address{ 0 };
			u32 _size{ 0 };

			constexpr void move(D3D12Buffer& o) {
				_buffer = o._buffer;
				_gpu_address = o._gpu_address;
				_size = o._size;
				o.reset();
			}

			constexpr void reset() {
				_buffer = nullptr;
				_gpu_address = 0;
				_size = 0;
			}
	};

	class ConstantBuffer {
		public:
			ConstantBuffer() = default;
			explicit ConstantBuffer(const D3D12BufferInitInfo& info);
			DISABLE_COPY_AND_MOVE(ConstantBuffer);

			void release() { 
				_buffer.release();
				_cpu_address = nullptr;
				_cpu_offset = 0;
			}

			constexpr void clear() { _cpu_offset = 0; }
			[[nodiscard]] u8* const allocate(u32 size);
			template<typename T> [[nodiscard]] constexpr T* const allocate() { return (T* const)allocate(sizeof(T)); }
			[[nodiscard]] constexpr ID3D12Resource* const buffer() const { return _buffer.buffer(); }
			[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return _buffer.gpu_address(); }
			[[nodiscard]] constexpr u32 size() const { return _buffer.size(); }
			[[nodiscard]] constexpr u8* const cpu_address() const { return _cpu_address; }

			template<typename T> [[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address(T* const allocate) {
				std::lock_guard lock{ _mutex };
				assert(_cpu_address);
				if (!_cpu_address) return {};
				const u8* const address{ (const u8* const)allocate };
				assert(address <= _cpu_address + _cpu_offset);
				assert(address >= _cpu_address);
				const u64 offset{ (u64)(address - _cpu_address) };
				return _buffer.gpu_address() + offset;
			}

			[[nodiscard]] constexpr static D3D12BufferInitInfo get_default_init_info(u32 size) {
				assert(size);
				D3D12BufferInitInfo info{};
				info.size = size;
				info.alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
				return info;
			}

			~ConstantBuffer() { release(); }
		private:
			D3D12Buffer _buffer{};
			u8* _cpu_address{ nullptr };
			u32 _cpu_offset{ 0 };
			std::mutex _mutex;
	};

	class UAVClearebleBuffer {
		public:
			UAVClearebleBuffer() = default;
			explicit UAVClearebleBuffer(const D3D12BufferInitInfo& info);
			DISABLE_COPY(UAVClearebleBuffer);

			constexpr UAVClearebleBuffer(UAVClearebleBuffer&& o) : _buffer{ std::move(o._buffer) }, _uav{ o._uav }, _uav_shader_visible{ o._uav_shader_visible } {
				o.reset();
			}

			constexpr UAVClearebleBuffer& operator=(UAVClearebleBuffer&& o) {
				assert(this != &o);
				if (this != &o) {
					release();
					move(o);
				}

				return *this;
			}

			~UAVClearebleBuffer() { release(); }

			void release();

			void clear_uav(id3d12_graphics_command_list* const cmd_list, const u32* const values) const {
				assert(buffer());
				assert(_uav.is_valid() && _uav_shader_visible.is_valid() && _uav_shader_visible.is_shader_visible());
				cmd_list->ClearUnorderedAccessViewUint(_uav_shader_visible.gpu, _uav.cpu, buffer(), values, 0, nullptr);
			}

			void clear_uav(id3d12_graphics_command_list* const cmd_list, const f32* const values) const {
				assert(buffer());
				assert(_uav.is_valid() && _uav_shader_visible.is_valid() && _uav_shader_visible.is_shader_visible());
				cmd_list->ClearUnorderedAccessViewFloat(_uav_shader_visible.gpu, _uav.cpu, buffer(), values, 0, nullptr);
			}

			[[nodiscard]] constexpr ID3D12Resource* buffer() const { return _buffer.buffer(); }
			[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return _buffer.gpu_address(); }
			[[nodiscard]] constexpr u32 size() const { return _buffer.size(); }
			[[nodiscard]] constexpr DescriptorHandle uav() const { return _uav; }
			[[nodiscard]] constexpr DescriptorHandle uav_shader_visible() const { return _uav_shader_visible; }

			[[nodiscard]] constexpr static D3D12BufferInitInfo get_default_init_info(u32 size) {
				assert(size);
				D3D12BufferInitInfo info{};
				info.size = size;
				info.alignment = sizeof(math::v4);
				info.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				return info;
			}

		private:

			constexpr void move(UAVClearebleBuffer& o) {
				_buffer = std::move(o._buffer);
				_uav = o._uav;
				_uav_shader_visible = o._uav_shader_visible;

				o.reset();
			}

			constexpr void reset() {
				_uav = {};
				_uav_shader_visible = {};
			}

			D3D12Buffer _buffer{};
			DescriptorHandle _uav{};
			DescriptorHandle _uav_shader_visible{};
	};

	struct D3D12TextureInitInfo {
		ID3D12Heap1* heap{ nullptr };
		ID3D12Resource2* resource{ nullptr };
		D3D12_SHADER_RESOURCE_VIEW_DESC* srv_desc{ nullptr };
		D3D12_RESOURCE_DESC* desc{ nullptr };
		D3D12_RESOURCE_ALLOCATION_INFO1 allocation_info{};
		D3D12_RESOURCE_STATES initial_state{};
		D3D12_CLEAR_VALUE clear_value{};
		DXGI_FORMAT format[1];
	};

	class D3D12Texture {
		public:
			constexpr static u32 max_mips{ 14 };

			D3D12Texture() = default;
			explicit D3D12Texture(D3D12TextureInitInfo info);
			DISABLE_COPY(D3D12Texture);
			constexpr D3D12Texture(D3D12Texture&& o) : _resource{ o._resource }, _srv{ o._srv } {
				o.reset();
			}

			constexpr D3D12Texture& operator=(D3D12Texture&& o) {
				assert(this != &o);
				if (this != &o) {
					release();
					move(o);
				}
				return *this;
			}

			~D3D12Texture() { release(); }

			void release();
			[[nodiscard]] constexpr ID3D12Resource2* const resource() const { return _resource; }
			[[nodiscard]] constexpr DescriptorHandle srv() const { return _srv; }

		private:
			ID3D12Resource2* _resource{ nullptr };
			DescriptorHandle _srv;

			constexpr void reset() {
				_resource = nullptr;
				_srv = {};
			}

			constexpr void move(D3D12Texture& o) {
				_resource = o._resource;
				_srv = o._srv;
				o.reset();
			}
	};

	class D3D12RenderTexture {
		public:
			D3D12RenderTexture() = default;
			explicit D3D12RenderTexture(D3D12TextureInitInfo info);
			DISABLE_COPY(D3D12RenderTexture);
			constexpr D3D12RenderTexture(D3D12RenderTexture&& o) : _texture{ std::move(o._texture) }, _mip_count{ o._mip_count } {
				for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = o._rtv[1];
				o.reset();
			}

			constexpr D3D12RenderTexture& operator=(D3D12RenderTexture&& o) {
				assert(this != &o);
				if (this != &o) {
					release();
					move(o);
				}
				return *this;
			}

			~D3D12RenderTexture() { release(); }

			void release();
			[[nodiscard]] constexpr u32 mip_count() const { return _mip_count; }
			[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv(u32 mip_index) const { return _rtv[mip_index].cpu; }
			[[nodiscard]] constexpr DescriptorHandle srv() const { return _texture.srv(); }
			[[nodiscard]] constexpr ID3D12Resource* const resource() const { return _texture.resource(); }

		private:
			constexpr void move(D3D12RenderTexture& o) {
				_texture = std::move(o._texture);
				_mip_count = o._mip_count;
				for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = o._rtv[i];
				o.reset();
			}

			constexpr void reset() {
				for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = {};
				_mip_count = 0;
			}

			D3D12Texture _texture{};
			DescriptorHandle _rtv[D3D12Texture::max_mips]{};
			u32 _mip_count{ 0 };
	};

	class D3D12DepthBuffer {
		public:
			D3D12DepthBuffer() = default;
			explicit D3D12DepthBuffer(D3D12TextureInitInfo info);
			DISABLE_COPY(D3D12DepthBuffer);
			constexpr D3D12DepthBuffer(D3D12DepthBuffer&& o) : _texture{ std::move(o._texture) }, _dsv{ o._dsv } {
				o._dsv = {};
			}

			constexpr D3D12DepthBuffer& operator=(D3D12DepthBuffer&& o) {
				assert(this != &o);
				if (this != &o) {
					_texture = std::move(o._texture);
					_dsv = o._dsv;
					o._dsv = {};
				}
				return *this;
			}

			~D3D12DepthBuffer() { release(); }

			void release();
			[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE dsv() const { return _dsv.cpu; }
			[[nodiscard]] constexpr DescriptorHandle srv() const { return _texture.srv(); }
			[[nodiscard]] constexpr ID3D12Resource* const resource() const { return _texture.resource(); }

		private:
			D3D12Texture _texture{};
			DescriptorHandle _dsv{};
	};
}