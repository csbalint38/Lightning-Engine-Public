#include "Direct3D12Upload.h"
#include "Direct3D12Core.h"

namespace lightning::graphics::direct3d12::upload {
	namespace {
		struct UploadFrame {
			ID3D12CommandAllocator* cmd_allocator{ nullptr };
			id3d12_graphics_command_list* cmd_list{ nullptr };
			ID3D12Resource* upload_buffer{ nullptr };
			void* cpu_address{ nullptr };
			u64 fence_value{ 0 };

			void wait_and_reset();
			void release() {
				wait_and_reset();
				core::release(cmd_allocator);
				core::release(cmd_list);
			}

			constexpr bool is_redy() const { return upload_buffer == nullptr; }
		};

		constexpr u32 upload_frame_count{ 4 };
		UploadFrame upload_frames[upload_frame_count]{};
		ID3D12CommandQueue* upload_cmd_queue{ nullptr };
		ID3D12Fence1* upload_fence{ nullptr };
		u64 upload_fence_value{ 0 };
		HANDLE fence_event{};
		std::mutex frame_mutex{};
		std::mutex queue_mutex{};

		void UploadFrame::wait_and_reset() {
			assert(upload_fence && fence_event);

			if (upload_fence->GetCompletedValue() < fence_value) {
				DXCall(upload_fence->SetEventOnCompletion(fence_value, fence_event));
				WaitForSingleObject(fence_event, INFINITE);
			}

			core::release(upload_buffer);
			cpu_address = nullptr;
		}

		u32 get_available_upload_frame() {
			u32 index{ u32_invalid_id };
			const u32 count{ upload_frame_count };
			UploadFrame* const frames{ &upload_frames[0] };

			for (u32 i{ 0 }; i < count; ++i) {
				if (frames[i].is_redy()) {
					index = i;
					break;
				}
			}

			if (index == u32_invalid_id) {
				index = 0;
				while (!frames[index].is_redy()) {
					index = (index + 1) % count;
					std::this_thread::yield();
				}
			}

			return index;
		}

		bool init_failed() {
			shutdown();
			return false;
		}
	}

	D3D12UploadContext::D3D12UploadContext(u32 aligned_size) {
		assert(upload_cmd_queue);
		{
			std::lock_guard lock{ frame_mutex };
			_frame_index = get_available_upload_frame();
			assert(_frame_index != u32_invalid_id);
			upload_frames[_frame_index].upload_buffer = (ID3D12Resource*)1;
		}

		UploadFrame& frame{ upload_frames[_frame_index] };
		frame.upload_buffer = d3dx::create_buffer(nullptr, aligned_size, true);
		NAME_D3D12_OBJECT_INDEXED(frame.upload_buffer, aligned_size, L"Upload Buffer - size");

		const D3D12_RANGE range{};
		DXCall(frame.upload_buffer->Map(0, &range, reinterpret_cast<void**>(&frame.cpu_address)));
		assert(frame.cpu_address);

		_cmd_list = frame.cmd_list;
		_upload_buffer = frame.upload_buffer;
		_cpu_address = frame.cpu_address;
		assert(_cmd_list && _upload_buffer && _cpu_address);

		DXCall(frame.cmd_allocator->Reset());
		DXCall(frame.cmd_list->Reset(frame.cmd_allocator, nullptr));
	}

	void D3D12UploadContext::end_upload() {
		assert(_frame_index != u32_invalid_id);
		UploadFrame& frame{ upload_frames[_frame_index] };
		id3d12_graphics_command_list* const cmd_list{ frame.cmd_list };
		DXCall(cmd_list->Close());

		std::lock_guard lock{ queue_mutex };

		ID3D12CommandList* const cmd_lists[]{ cmd_list };
		ID3D12CommandQueue* const cmd_queue{ upload_cmd_queue };
		cmd_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);

		++upload_fence_value;
		frame.fence_value = upload_fence_value;
		DXCall(cmd_queue->Signal(upload_fence, frame.fence_value));

		frame.wait_and_reset();
		DEBUG_OP(new (this)D3D12UploadContext{});
	}

	bool initialize() {
		id3d12_device* const device{ core::device() };
		assert(device && !upload_cmd_queue);

		HRESULT hr{ S_OK };

		for (u32 i{ 0 }; i < upload_frame_count; ++i) {
			UploadFrame& frame{ upload_frames[i] };
			DXCall(hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&frame.cmd_allocator)));
			if (FAILED(hr)) return init_failed();

			DXCall(hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, frame.cmd_allocator, nullptr, IID_PPV_ARGS(&frame.cmd_list)));
			if (FAILED(hr)) return init_failed();

			DXCall(frame.cmd_list->Close());

			NAME_D3D12_OBJECT_INDEXED(frame.cmd_allocator, i, L"Upload Command Allocator");
			NAME_D3D12_OBJECT_INDEXED(frame.cmd_list, i, L"Upload Command List");
		}

		D3D12_COMMAND_QUEUE_DESC desc{};
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

		DXCall(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&upload_cmd_queue)));
		if (FAILED(hr)) return init_failed();
		NAME_D3D12_OBJECT(upload_cmd_queue, L"Upload Copy Queue");

		DXCall(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&upload_fence)));
		if (FAILED(hr)) return init_failed();

		NAME_D3D12_OBJECT(upload_fence, L"Upload Fence");

		fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		assert(fence_event);
		if (!fence_event) return init_failed();

		return true;
	}

	void shutdown() {
		for (u32 i{ 0 }; i < upload_frame_count; ++i) {
			upload_frames[i].release();
		}

		if (fence_event) {
			CloseHandle(fence_event);
			fence_event = nullptr;
		}

		core::release(upload_cmd_queue);
		core::release(upload_fence);
		upload_fence_value = 0;
	}
}