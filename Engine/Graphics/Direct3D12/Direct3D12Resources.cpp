#include "Direct3D12Resources.h"
#include "Direct3D12Core.h"
#include "Direct3D12Helpers.h"

namespace lightning::graphics::direct3d12 {

	#pragma region DESCRIPTOR HEAP

	bool DescriptorHeap::initialize(u32 capacity, bool is_shader_visible) {
		std::lock_guard lock{ _mutex };
		assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
		assert(!(_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));
		if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
			is_shader_visible = false;
		}

		release();

		id3d12_device* const device{ core::device() };
		assert(device);

		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Flags = is_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NumDescriptors = capacity;
		desc.Type = _type;
		desc.NodeMask = 0;

		HRESULT hr{ S_OK };
		DXCall(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
		if (FAILED(hr)) return false;

		_free_handles = std::make_unique<u32[]>(capacity);
		_capacity = capacity;
		_size = 0;

		for (u32 i{ 0 }; i < capacity; ++i) _free_handles[i] = i;
		DEBUG_OP(for (u32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i) assert(_deferred_free_indicies[i].empty()));

		_descriptor_size = device->GetDescriptorHandleIncrementSize(_type);
		_cpu_start = _heap->GetCPUDescriptorHandleForHeapStart();
		_gpu_start = is_shader_visible ? _heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

		return true;
	}

	void DescriptorHeap::release() {
		assert(!_size);
		core::deferred_release(_heap);
	}

	void DescriptorHeap::process_deferred_free(u32 frame_idx) {
		std::lock_guard lock{ _mutex };
		assert(frame_idx < FRAME_BUFFER_COUNT);

		util::vector<u32>& indicies{ _deferred_free_indicies[frame_idx] };

		if (!indicies.empty()) {
			for (auto index : indicies) {
				--_size;
				_free_handles[_size] = index;
			}
			indicies.clear();
		}
	}

	DescriptorHandle DescriptorHeap::allocate() {
		std::lock_guard lock{ _mutex };
		assert(_heap);
		assert(_size < _capacity);

		const u32 index{ _free_handles[_size] };
		const u32 offset{ index * _descriptor_size };
		++_size;

		DescriptorHandle handle;
		handle.cpu.ptr = _cpu_start.ptr + offset;
		if (is_shader_visible()) {
			handle.gpu.ptr = _gpu_start.ptr + offset;
		}

		handle.index = index;
		DEBUG_OP(handle.container = this);
		return handle;
	}

	void DescriptorHeap::free(DescriptorHandle& handle) {
		if (!handle.is_valid()) return;
		std::lock_guard lock{ _mutex };

		assert(_heap && _size);
		assert(handle.container == this);
		assert(handle.cpu.ptr >= _cpu_start.ptr);
		assert((handle.cpu.ptr - _cpu_start.ptr) % _descriptor_size == 0);
		assert(handle.index < _capacity);
		const u32 index{ (u32)(handle.cpu.ptr - _cpu_start.ptr) / _descriptor_size };
		assert(handle.index == index);

		const u32 frame_idx{ core::current_frame_index() };
		_deferred_free_indicies[frame_idx].push_back(index);
		core::set_deferred_release_flag();

		handle = {};
	}
	#pragma endregion

	#pragma region D3D12_BUFFER

	D3D12Buffer::D3D12Buffer(const D3D12BufferInitInfo& info, bool is_cpu_accessible) {
		assert(!_buffer && info.size && info.alignment);

		_size = (u32)math::align_size_up(info.size, info.alignment);
		_buffer = d3dx::create_buffer(info.data, _size, is_cpu_accessible, info.initial_state, info.flags, info.heap, info.allocation_info.Offset);
		_gpu_address = _buffer->GetGPUVirtualAddress();
		NAME_D3D12_OBJECT_INDEXED(_buffer, _size, L"D3D12Buffer - size");
	}

	void D3D12Buffer::release() {
		core::deferred_release(_buffer);
		_gpu_address = 0;
		_size = 0;
	}

	#pragma endregion

	#pragma region D3D12_CONSTANT_BUFFER

	ConstantBuffer::ConstantBuffer(const D3D12BufferInitInfo& info) : _buffer{ info, true } {
		NAME_D3D12_OBJECT_INDEXED(buffer(), size(), L"Constant Buffer - size");

		D3D12_RANGE range{};
		DXCall(buffer()->Map(0, &range, (void**)(&_cpu_address)));
		assert(_cpu_address);
	}

	u8* const ConstantBuffer::allocate(u32 size) {
		std::lock_guard lock{ _mutex };

		const u32 aligned_size{ (u32)d3dx::align_size_for_constant_buffer(size) };
		assert(_cpu_offset + aligned_size <= _buffer.size());

		if (_cpu_offset + aligned_size <= _buffer.size()) {
			u8* const address{ _cpu_address + _cpu_offset };
			_cpu_offset += aligned_size;

			return address;
		}

		return nullptr;
	}

	#pragma endregion

	#pragma region UAV_CLEARABLE_BUFFER

	UAVClearebleBuffer::UAVClearebleBuffer(const D3D12BufferInitInfo& info) : _buffer{ info, false } {
		assert(info.size && info.alignment);

		NAME_D3D12_OBJECT_INDEXED(buffer(), info.size, L"UAV Clearable Buffer - size");

		assert(info.flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		_uav = core::uav_heap().allocate();
		_uav_shader_visible = core::srv_heap().allocate();
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
		desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_R32_UINT;
		desc.Buffer.CounterOffsetInBytes = 0;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		desc.Buffer.NumElements = _buffer.size() / sizeof(u32);

		core::device()->CreateUnorderedAccessView(buffer(), nullptr, &desc, _uav.cpu);
		core::device()->CopyDescriptorsSimple(1, _uav_shader_visible.cpu, _uav.cpu, core::srv_heap().type());
	}

	void UAVClearebleBuffer::release() {
		core::srv_heap().free(_uav_shader_visible);
		core::uav_heap().free(_uav);
		_buffer.release();
	}

	#pragma endregion

	#pragma region D3D12_TEXTURE
	D3D12Texture::D3D12Texture(D3D12TextureInitInfo info) {
		auto* const device{ core::device() };
		assert(device);

		D3D12_CLEAR_VALUE* const clear_value{
			(info.desc && (info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET || info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) ? &info.clear_value : nullptr
		};

		if (info.resource) {
			_resource = info.resource;
		}
		else if (info.heap) {
			assert(info.desc);
			DXCall(device->CreatePlacedResource(info.heap, info.allocation_info.Offset, info.desc, info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
		}
		else {
			assert(info.desc);

			DXCall(device->CreateCommittedResource(&d3dx::heap_properties.default_heap, D3D12_HEAP_FLAG_NONE, info.desc, info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
		}

		assert(_resource);
		_srv = core::srv_heap().allocate();
		device->CreateShaderResourceView(_resource, info.srv_desc, _srv.cpu);
	}

	void D3D12Texture::release() {
		core::srv_heap().free(_srv);
		core::deferred_release(_resource);
	}
	#pragma endregion

	#pragma region RENDER_TEXTURE
	D3D12RenderTexture::D3D12RenderTexture(D3D12TextureInitInfo info) : _texture{ info } {
		assert(info.desc);
		_mip_count = resource()->GetDesc().MipLevels;
		assert(_mip_count && _mip_count <= D3D12Texture::max_mips);

		DescriptorHeap& rtv_heap{ core::rtv_heap() };
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = info.desc->Format;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		auto* const device{ core::device() };
		assert(device);

		for (u32 i{ 0 }; i < _mip_count; ++i) {
			_rtv[i] = rtv_heap.allocate();
			device->CreateRenderTargetView(resource(), &desc, _rtv[i].cpu);
			++desc.Texture2D.MipSlice;
		}
	}

	void D3D12RenderTexture::release() {
		for (u32 i{ 0 }; i < _mip_count; ++i) core::rtv_heap().free(_rtv[i]);
		_texture.release();
		_mip_count = 0;
	}
	#pragma endregion

	#pragma region DEPTH_BUFFER
	D3D12DepthBuffer::D3D12DepthBuffer(D3D12TextureInitInfo info) {
		assert(info.desc);
		const DXGI_FORMAT dsv_format{ info.desc->Format };

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
		if (info.desc->Format == DXGI_FORMAT_D32_FLOAT) {
			info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
			srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
		}

		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Texture2D.MostDetailedMip = 0;
		srv_desc.Texture2D.PlaneSlice = 0;
		srv_desc.Texture2D.ResourceMinLODClamp = 0.f;

		assert(!info.srv_desc && !info.resource);
		info.srv_desc = &srv_desc;
		_texture = D3D12Texture(info);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
		dsv_desc.Format = dsv_format;
		dsv_desc.Texture2D.MipSlice = 0;

		_dsv = core::dsv_heap().allocate();

		auto* const device{ core::device() };
		assert(device);
		device->CreateDepthStencilView(resource(), &dsv_desc, _dsv.cpu);
	}

	void D3D12DepthBuffer::release() {
		core::dsv_heap().free(_dsv);
		_texture.release();
	}
	#pragma endregion
}