/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "common/size_definitions.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/descriptor_handle.hpp"
#include "data/vector.hpp"
#include "data/static_vector.hpp"
#include "data/span.hpp"
#include "data/string.hpp"
#include "data/bitmask.hpp"
#include "memory/pool_allocator.hpp"
#include "dx12_heap.hpp"
#include <wrl/client.h>
#include <dxgi1_6.h>
#include "gfx/backend/dx12/sdk/d3dx12.h"

namespace D3D12MA
{
	class Allocator;
	class Allocation;
} // namespace D3D12MA

struct IDxcLibrary;
struct IDXGISwapChain3;
struct IDXGIAdapter1;

namespace SFG
{
	struct resource_desc;
	struct texture_desc;
	struct sampler_desc;
	struct swapchain_desc;
	struct swapchain_recreate_desc;
	struct shader_desc;
	struct bind_group_desc;
	struct command_buffer_desc;
	struct queue_desc;
	struct bind_group_update_desc;
	struct bind_layout_desc;
	struct bind_layout_pointer_param;
	struct bind_group_pointer;
	struct shader_blob;
	struct shader_blob;
	struct command_begin_render_pass;
	struct command_begin_render_pass_depth;
	struct command_begin_render_pass_depth_only;
	struct command_begin_render_pass_swapchain;
	struct command_begin_render_pass_swapchain_depth;
	struct command_end_render_pass;
	struct command_set_scissors;
	struct command_set_viewport;
	struct command_bind_pipeline;
	struct command_bind_pipeline_compute;
	struct command_draw_instanced;
	struct command_draw_instanced;
	struct command_draw_indexed_instanced;
	struct command_draw_indexed_indirect;
	struct command_draw_indirect;
	struct command_bind_vertex_buffers;
	struct command_bind_index_buffers;
	struct command_copy_resource;
	struct command_copy_resource_region;
	struct command_copy_buffer_to_texture;
	struct command_copy_texture_to_buffer;
	struct command_copy_texture_to_texture;
	struct command_bind_constants;
	struct command_bind_layout;
	struct command_bind_layout_compute;
	struct command_bind_group;
	struct command_dispatch;
	struct command_barrier;

	struct command_bind_group;

#ifdef SFG_DEBUG
#define BEGIN_DEBUG_EVENT(backend, CMD_BUF, LABEL) backend->cmd_begin_event(CMD_BUF, LABEL)
#define END_DEBUG_EVENT(backend, CMD_BUF)		   backend->cmd_end_event(CMD_BUF)
#else
#define BEGIN_DEBUG_EVENT(backend, CMD_BUF, LABEL)
#define END_DEBUG_EVENT(backend, CMD_BUF)
#endif

	class dx12_backend
	{
	private:
		struct resource
		{
			D3D12MA::Allocation* ptr						= nullptr;
			i16				 descriptor_index			= -1;
			i16				 descriptor_index_secondary = -1;
			u32				 size						= 0;
		};

		struct texture_view
		{
			gfx_id handle = 0;
			u8  type	  = 0;
		};

		struct texture
		{
			D3D12MA::Allocation* ptr = nullptr;
			texture_view		 views[8];
			gfx_id				 shared_handle = 0;
			u8				 format		   = 0;
			u8				 view_count	   = 0;
		};

		struct texture_shared_handle
		{
			HANDLE handle = 0;
		};

		struct sampler
		{
			gfx_id descriptor_index = 0;
		};

		struct swapchain
		{
			Microsoft::WRL::ComPtr<IDXGISwapChain3> ptr = NULL;
			Microsoft::WRL::ComPtr<ID3D12Resource>	textures[BACK_BUFFER_COUNT];
#ifdef SFG_ENABLE_MEMORY_TRACER
			u32 size = 0;
#endif
			gfx_id rtv_indices[BACK_BUFFER_COUNT] = {};
			u8  format						  = 0;
			u8  image_index					  = 0;
			u8  vsync						  = 0;
			u8  tearing						  = 0;
			HANDLE frame_latency_waitable		  = NULL;
		};

		struct semaphore
		{
			Microsoft::WRL::ComPtr<ID3D12Fence> ptr = nullptr;
		};

		struct shader
		{
			Microsoft::WRL::ComPtr<ID3D12PipelineState> ptr				   = nullptr;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature	   = nullptr;
			u8										indirect_signature = 0;
			u8										topology		   = 0;
			u8										owns_root_sig	   = 0;
		};

		struct indirect_signature
		{
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> signature = nullptr;
		};

		struct group_binding
		{
			u8* constants		= nullptr;
			gfx_id descriptor_index = 0;
			u32 root_param_index = 0;
			u8  binding_type		= 0;
			u8  count			= 0;
		};

		struct bind_group
		{
			vector<group_binding> bindings;
		};

		struct command_buffer
		{
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> ptr;
			gfx_id											   allocator   = 0;
			u8											   is_transfer = 0;
		};

		struct command_allocator
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> ptr;
		};

		struct queue
		{
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> ptr;
		};

		struct bind_layout
		{
			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature = nullptr;
		};

	public:
		inline static dx12_backend* get()
		{
			return s_instance;
		}

		bool  init();
		void  uninit();
		void  reset_command_buffer(gfx_id cmd_buffer);
		void  reset_command_buffer_transfer(gfx_id cmd_buffer);
		void  close_command_buffer(gfx_id cmd_buffer);
		void  submit_commands(gfx_id queue, const gfx_id* commands, u8 commands_count);
		void  queue_wait(gfx_id queue, const gfx_id* semaphores, const u64* semaphore_values, u8 semaphore_count);
		void  queue_signal(gfx_id queue, const gfx_id* semaphores, const u64* semaphore_values, u8 semaphore_count);
		void  present(const gfx_id* swapchains, u8 swapchain_count);
		u8 get_back_buffer_index(gfx_id swapchain);
		void  wait_for_swapchain_latency(gfx_id swapchain);

		bool compile_shader_vertex_pixel(u8 stage, const string& source, const vector<string>& defines, const vector<string>& source_paths, const char* entry, span<u8>& out, bool compile_layout, span<u8>& out_layout) const;
		bool compile_shader_compute(const string& source, const vector<string>& source_paths, const char* entry, span<u8>& out, bool compile_layout, span<u8>& out_layout) const;

		u32 get_resource_gpu_index(gfx_id resource, bool use_secondary = false);
		u32 get_texture_gpu_index(gfx_id texture, u8 view_index);
		u32 get_sampler_gpu_index(gfx_id sampler);
		gfx_id create_resource(const resource_desc& desc);
		gfx_id create_texture(const texture_desc& desc);
		gfx_id create_sampler(const sampler_desc& desc);
		gfx_id create_swapchain(const swapchain_desc&);
		gfx_id recreate_swapchain(const swapchain_recreate_desc& desc);
		gfx_id create_semaphore();
		gfx_id create_shader(const shader_desc& desc, const vector<shader_blob>& blobs, gfx_id existing_layout, span<u8> layout_data = {});
		gfx_id create_empty_bind_group();
		gfx_id create_command_buffer(const command_buffer_desc& desc);
		gfx_id create_command_allocator(u8 ctype);
		gfx_id create_queue(const queue_desc& desc);
		gfx_id create_empty_bind_layout();
		gfx_id create_draw_indirect_signature(gfx_id bind_layout, size_t sz);
		gfx_id create_dispatch_indirect_signature(gfx_id bind_layout, size_t sz);
		void   destroy_indirect_signature(gfx_id sig);
		void   bind_group_add_descriptor(gfx_id group, u8 root_param_index, u8 binding_type);
		void   bind_group_add_constant(gfx_id group, u8 root_param_index, u8* data, u8 count);
		void   bind_group_add_pointer(gfx_id group, u8 root_param_index, u8 count, bool is_sampler);
		void   bind_layout_add_constant(gfx_id layout, u32 count, u32 set, u32 binding, u8 shader_stage_visibility);
		void   bind_layout_add_descriptor(gfx_id layout, u8 type, u32 set, u32 binding, u8 shader_stage_visibility);
		void   bind_layout_add_pointer(gfx_id layout, const vector<bind_layout_pointer_param>& pointer_params, u8 shader_stage_visibility);
		void   bind_layout_add_immutable_sampler(gfx_id layout, u32 set, u32 binding, const sampler_desc& desc, u8 shader_stage_visibility);
		void   finalize_bind_layout(gfx_id id, bool is_compute, bool is_dyn_index, const char* name);
		void   bind_group_update_constants(gfx_id group, u8 binding_index, u8* constants, u8 count);
		void   bind_group_update_descriptor(gfx_id group, u8 binding_index, gfx_id resource);
		void   bind_group_update_pointer(gfx_id group, u8 binding_index, const bind_group_pointer* updates, u16 update_count);
		void   bind_group_update_pointer(gfx_id group, u8 binding_index, const vector<bind_group_pointer>& updates);

		void destroy_resource(gfx_id id);
		void destroy_texture(gfx_id id);
		void destroy_sampler(gfx_id id);
		void destroy_swapchain(gfx_id id);
		void destroy_semaphore(gfx_id id);
		void destroy_shader(gfx_id id);
		void destroy_bind_group(gfx_id id);
		void destroy_command_buffer(gfx_id id);
		void destroy_command_allocator(gfx_id id);
		void destroy_queue(gfx_id id);
		void destroy_bind_layout(gfx_id id);

		void wait_semaphore(gfx_id id, u64 value) const;
		void map_resource(gfx_id id, u8*& ptr) const;
		void unmap_resource(gfx_id id) const;

		inline HANDLE get_swapchain_latency_handle(gfx_id id)
		{
			return _swapchains.get(id).frame_latency_waitable;
		}

		HANDLE get_shared_handle_for_texture(gfx_id id);

		u32 get_texture_size(u32 width, u32 height, u32 bpp) const;
		u32 align_texture_size(u32 size) const;
		u32 align_texture_size_pitch(u32 size) const;
		void*  adjust_buffer_pitch(void* data, u32 width, u32 height, u8 bpp, u32& out_total_size) const;

		void cmd_begin_event(gfx_id cmd_list, const char* label);
		void cmd_end_event(gfx_id cmd_list);
		void cmd_begin_render_pass(gfx_id cmd_list, const command_begin_render_pass& command);
		void cmd_begin_render_pass_depth(gfx_id cmd_list, const command_begin_render_pass_depth& command);
		void cmd_begin_render_pass_depth_read_only(gfx_id cmd_list, const command_begin_render_pass_depth& command);
		void cmd_begin_render_pass_depth_only(gfx_id cmd_list, const command_begin_render_pass_depth_only& command);
		void cmd_begin_render_pass_swapchain(gfx_id cmd_list, const command_begin_render_pass_swapchain& command);
		void cmd_begin_render_pass_swapchain_depth(gfx_id cmd_list, const command_begin_render_pass_swapchain_depth& command);
		void cmd_end_render_pass(gfx_id cmd_list, const command_end_render_pass& command) const;
		void cmd_set_scissors(gfx_id cmd_list, const command_set_scissors& command) const;
		void cmd_set_viewport(gfx_id cmd_list, const command_set_viewport& command) const;
		void cmd_bind_pipeline(gfx_id cmd_list, const command_bind_pipeline& command) const;
		void cmd_bind_pipeline_compute(gfx_id cmd_list, const command_bind_pipeline_compute& command) const;
		void cmd_draw_instanced(gfx_id cmd_list, const command_draw_instanced& command) const;
		void cmd_draw_indexed_instanced(gfx_id cmd_list, const command_draw_indexed_instanced& command) const;
		void cmd_execute_indirect(gfx_id cmd_list, const command_draw_indirect& command) const;
		void cmd_bind_vertex_buffers(gfx_id cmd_list, const command_bind_vertex_buffers& command) const;
		void cmd_bind_index_buffers(gfx_id cmd_list, const command_bind_index_buffers& command) const;
		void cmd_copy_resource(gfx_id cmd_list, const command_copy_resource& command) const;
		void cmd_copy_resource_region(gfx_id cmd_list, const command_copy_resource_region& command) const;
		void cmd_copy_buffer_to_texture(gfx_id cmd_list, const command_copy_buffer_to_texture& command);
		void cmd_copy_texture_to_buffer(gfx_id cmd_list, const command_copy_texture_to_buffer& command) const;
		void cmd_copy_texture_to_texture(gfx_id cmd_list, const command_copy_texture_to_texture& command) const;
		void cmd_bind_constants(gfx_id cmd_list, const command_bind_constants& command) const;
		void cmd_bind_constants_compute(gfx_id cmd_list, const command_bind_constants& command) const;
		void cmd_bind_layout(gfx_id cmd_list, const command_bind_layout& command) const;
		void cmd_bind_layout_compute(gfx_id cmd_list, const command_bind_layout_compute& command) const;
		void cmd_bind_group(gfx_id cmd_list, const command_bind_group& command) const;
		void cmd_bind_group_compute(gfx_id cmd_list, const command_bind_group& command) const;
		void cmd_dispatch(gfx_id cmd_list, const command_dispatch& command) const;
		void cmd_barrier(gfx_id cmd_list, const command_barrier& command);

		ID3D12DescriptorHeap* get_srv_heap();
		void				  alloc_srv(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle);
		void				  free_srv(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);
		u64				  get_srv_gpu_handle_from_index(u32 index);

		inline gfx_id get_queue_gfx() const
		{
			return _queue_graphics;
		}

		inline gfx_id get_queue_transfer() const
		{
			return _queue_transfer;
		}

		inline gfx_id get_queue_compute() const
		{
			return _queue_compute;
		}

		inline ID3D12Device* get_device() const
		{
			return _device.Get();
		}

		inline ID3D12GraphicsCommandList4* get_gfx_cmd_list(u32 list)
		{
			return _command_buffers.get(list).ptr.Get();
		}

		inline ID3D12CommandQueue* get_queue_gfx_impl() const
		{
			return _queues.get(_queue_graphics).ptr.Get();
		}

	private:
		void wait_for_fence(ID3D12Fence* fence, u64 value) const;

	private:
		pool_allocator<resource, gfx_id, MAX_RESOURCES>					  _resources;
		pool_allocator<texture, gfx_id, MAX_TEXTURES>					  _textures;
		pool_allocator<texture_shared_handle, gfx_id, MAX_TEXTURES>		  _texture_shared_handles;
		pool_allocator<sampler, gfx_id, MAX_SAMPLERS>					  _samplers;
		pool_allocator<swapchain, gfx_id, MAX_SWAPCHAINS>				  _swapchains;
		pool_allocator<semaphore, gfx_id, MAX_SEMAPHORES>				  _semaphores;
		pool_allocator<shader, gfx_id, MAX_SHADERS>						  _shaders;
		pool_allocator<bind_group, gfx_id, MAX_BIND_GROUPS>				  _bind_groups;
		pool_allocator<command_buffer, gfx_id, MAX_COMMAND_BUFFERS>		  _command_buffers;
		pool_allocator<command_allocator, gfx_id, MAX_COMMAND_BUFFERS>	  _command_allocators;
		pool_allocator<queue, gfx_id, MAX_QUEUES>						  _queues;
		pool_allocator<indirect_signature, gfx_id, 255>					  _indirect_signatures;
		pool_allocator<descriptor_handle, gfx_id, MAX_DESCRIPTOR_HANDLES> _descriptors;
		pool_allocator<bind_layout, gfx_id, MAX_BIND_LAYOUTS>			  _bind_layouts;

		dx12_heap _heap_rtv			= {};
		dx12_heap _heap_dsv			= {};
		dx12_heap _heap_gpu_buffer	= {};
		dx12_heap _heap_gpu_sampler = {};

		D3D12MA::Allocator*						   _allocator = nullptr;
		Microsoft::WRL::ComPtr<IDXGIAdapter1>	   _adapter	  = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Device>	   _device	  = nullptr;
		Microsoft::WRL::ComPtr<IDXGIFactory4>	   _factory	  = nullptr;
		static Microsoft::WRL::ComPtr<IDxcLibrary> s_idxcLib;

		gfx_id _queue_graphics	  = 0;
		gfx_id _queue_transfer	  = 0;
		gfx_id _queue_compute	  = 0;
		bool   _tearing_supported = false;

		vector<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_dest_descriptors_buffer	= {};
		vector<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_dest_descriptors_sampler = {};
		vector<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_src_descriptors_buffer	= {};
		vector<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_src_descriptors_sampler	= {};
		vector<CD3DX12_ROOT_PARAMETER1>		_reuse_root_params				= {};
		vector<CD3DX12_DESCRIPTOR_RANGE1>	_reuse_root_ranges				= {};
		vector<D3D12_STATIC_SAMPLER_DESC>	_reuse_static_samplers			= {};

		static_vector<descriptor_handle, 24> _external_descriptors;

		friend class app;

		static dx12_backend* s_instance;
	};
}
