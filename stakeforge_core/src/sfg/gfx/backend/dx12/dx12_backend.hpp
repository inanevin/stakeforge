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

#include "dx12_heap.hpp"
#include "sdk/d3dx12.h"
#include <sfg/common/size_definitions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/descriptor_handle.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/bitmask.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <wrl/client.h>
#include <dxgi1_6.h>

namespace D3D12MA
{
	class Allocator;
	class Allocation;
} // namespace D3D12MA

struct IDxcLibrary;
struct IDXGISwapChain3;
struct IDXGIAdapter1;

namespace sfg
{
	struct resource_desc_t;
	struct texture_desc_t;
	struct sampler_desc_t;
	struct swapchain_desc_t;
	struct swapchain_recreate_desc_t;
	struct shader_desc_t;
	struct bind_group_desc_t;
	struct command_buffer_desc_t;
	struct queue_desc_t;
	struct bind_group_update_desc_t;
	struct bind_layout_desc_t;
	struct bind_layout_pointer_param_t;
	struct bind_group_pointer_t;
	struct shader_blob_t;
	struct shader_blob_t;
	struct command_begin_render_pass_t;
	struct command_begin_render_pass_depth_t;
	struct command_begin_render_pass_depth_only_t;
	struct command_begin_render_pass_swapchain_t;
	struct command_begin_render_pass_swapchain_depth_t;
	struct command_end_render_pass_t;
	struct command_set_scissors_t;
	struct command_set_viewport_t;
	struct command_bind_pipeline_t;
	struct command_bind_pipeline_compute_t;
	struct command_draw_instanced_t;
	struct command_draw_instanced_t;
	struct command_draw_indexed_instanced_t;
	struct command_draw_indexed_indirect_t;
	struct command_draw_indirect_t;
	struct command_bind_vertex_buffers_t;
	struct command_bind_index_buffers_t;
	struct command_copy_resource_t;
	struct command_copy_resource_region_t;
	struct command_copy_buffer_to_texture_t;
	struct command_copy_buffer_region_to_texture_t;
	struct command_copy_texture_to_buffer_t;
	struct command_copy_texture_to_texture_t;
	struct command_bind_constants_t;
	struct command_bind_layout_t;
	struct command_bind_layout_compute_t;
	struct command_bind_group_t;
	struct command_dispatch_t;
	struct command_barrier_t;

	struct command_bind_group_t;

#ifdef SFG_DEBUG
#define BEGIN_DEBUG_EVENT(backend, CMD_BUF, LABEL) backend->cmd_begin_event(CMD_BUF, LABEL)
#define END_DEBUG_EVENT(backend, CMD_BUF)		   backend->cmd_end_event(CMD_BUF)
#else
#define BEGIN_DEBUG_EVENT(backend, CMD_BUF, LABEL)
#define END_DEBUG_EVENT(backend, CMD_BUF)
#endif

	class dx12_backend_t
	{
	private:
		struct resource_t
		{
			D3D12MA::Allocation*  ptr						 = nullptr;
			gfx_descriptor_handle descriptor_index			 = {};
			gfx_descriptor_handle descriptor_index_secondary = {};
			u32					  size						 = 0;
		};

		struct texture_view_t
		{
			gfx_descriptor_handle handle = {};
			u8					  type	 = 0;
		};

		struct texture_t
		{
			D3D12MA::Allocation*	  ptr = nullptr;
			texture_view_t			  views[8];
			gfx_texture_shared_handle shared_handle = {};
			u32						  state			= resource_state_common;
			u8						  format		= 0;
			u8						  view_count	= 0;
		};

		struct texture_shared_handle_t
		{
			HANDLE handle = 0;
		};

		struct sampler_t
		{
			gfx_descriptor_handle descriptor_index = {};
		};

		struct swapchain_t
		{
			Microsoft::WRL::ComPtr<IDXGISwapChain3> ptr = NULL;
			Microsoft::WRL::ComPtr<ID3D12Resource>	textures[BACK_BUFFER_COUNT];
#ifdef SFG_ENABLE_MEMORY_TRACER
			u32 size = 0;
#endif
			gfx_descriptor_handle rtv_indices[BACK_BUFFER_COUNT] = {};
			u8					  format						 = 0;
			u8					  image_index					 = 0;
			u8					  vsync							 = 0;
			u8					  tearing						 = 0;
			HANDLE				  frame_latency_waitable		 = NULL;
		};

		struct semaphore_t
		{
			Microsoft::WRL::ComPtr<ID3D12Fence> ptr = nullptr;
		};

		struct shader_t
		{
			Microsoft::WRL::ComPtr<ID3D12PipelineState> ptr					 = nullptr;
			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature		 = nullptr;
			u8											indirect_signature_t = 0;
			u8											topology			 = 0;
			u8											owns_root_sig		 = 0;
		};

		struct indirect_signature_t
		{
			Microsoft::WRL::ComPtr<ID3D12CommandSignature> signature = nullptr;
		};

		struct group_binding_t
		{
			u8*					  constants		   = nullptr;
			gfx_descriptor_handle descriptor_index = {};
			u32					  root_param_index = 0;
			u8					  binding_type	   = 0;
			u8					  count			   = 0;
		};

		struct bind_group_t
		{
			vector_t<group_binding_t> bindings;
		};

		struct command_buffer_t
		{
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> ptr;
			gfx_command_allocator_handle					   allocator   = {};
			u8												   is_transfer = 0;
		};

		struct command_allocator_t
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> ptr;
		};

		struct queue_t
		{
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> ptr;
		};

		struct bind_layout_t
		{
			Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature = nullptr;
		};

	public:
		inline static dx12_backend_t& get()
		{
			static dx12_backend_t instance;
			return instance;
		}

		bool init();
		void uninit();
		void reset_command_buffer(gfx_command_buffer_handle cmd_buffer);
		void reset_command_buffer_transfer(gfx_command_buffer_handle cmd_buffer);
		void close_command_buffer(gfx_command_buffer_handle cmd_buffer);
		void submit_commands(gfx_queue_handle queue_t, const gfx_command_buffer_handle* commands, u8 commands_count);
		void queue_wait(gfx_queue_handle queue_t, const gfx_semaphore_handle* semaphores, const u64* semaphore_values, u8 semaphore_count);
		void queue_signal(gfx_queue_handle queue_t, const gfx_semaphore_handle* semaphores, const u64* semaphore_values, u8 semaphore_count);
		void present(const gfx_swapchain_handle* swapchains, u8 swapchain_count);
		u8	 get_back_buffer_index(gfx_swapchain_handle swapchain_t);
		void wait_for_swapchain_latency(gfx_swapchain_handle swapchain_t);

		static bool compile_shader_vertex_pixel(u8 stage, const string_t& source, const vector_t<string_t>& defines, const vector_t<string_t>& source_paths, const char* entry_t, span_t<u8>& out, bool compile_layout, span_t<u8>& out_layout);
		static bool compile_shader_compute(const string_t& source, const vector_t<string_t>& source_paths, const char* entry_t, span_t<u8>& out, bool compile_layout, span_t<u8>& out_layout);

		u32							  get_resource_gpu_index(gfx_resource_handle resource_t, bool use_secondary = false);
		u32							  get_texture_gpu_index(gfx_texture_handle texture_t, u8 view_index);
		u32							  get_texture_state(gfx_texture_handle texture_t) const;
		void						  set_texture_state(gfx_texture_handle texture_t, u32 state);
		u32							  get_sampler_gpu_index(gfx_sampler_handle sampler_t);
		gfx_resource_handle			  create_resource(const resource_desc_t& desc);
		gfx_texture_handle			  create_texture(const texture_desc_t& desc);
		gfx_sampler_handle			  create_sampler(const sampler_desc_t& desc);
		gfx_swapchain_handle		  create_swapchain(const swapchain_desc_t&);
		gfx_swapchain_handle		  recreate_swapchain(const swapchain_recreate_desc_t& desc);
		gfx_semaphore_handle		  create_semaphore();
		gfx_shader_handle			  create_shader(const shader_desc_t& desc, span_t<const shader_blob_t> blobs, gfx_bind_layout_handle existing_layout = {});
		gfx_bind_group_handle		  create_empty_bind_group();
		gfx_command_buffer_handle	  create_command_buffer(const command_buffer_desc_t& desc);
		gfx_command_allocator_handle  create_command_allocator(u8 ctype);
		gfx_queue_handle			  create_queue(const queue_desc_t& desc);
		gfx_bind_layout_handle		  create_empty_bind_layout();
		gfx_indirect_signature_handle create_draw_indirect_signature(gfx_bind_layout_handle bind_layout_t, size_t sz);
		gfx_indirect_signature_handle create_dispatch_indirect_signature(gfx_bind_layout_handle bind_layout_t, size_t sz);
		void						  destroy_indirect_signature(gfx_indirect_signature_handle sig);
		void						  bind_group_add_descriptor(gfx_bind_group_handle group, u8 root_param_index, u8 binding_type);
		void						  bind_group_add_constant(gfx_bind_group_handle group, u8 root_param_index, u8* data, u8 count);
		void						  bind_group_add_pointer(gfx_bind_group_handle group, u8 root_param_index, u8 count, bool is_sampler);
		void						  bind_layout_add_constant(gfx_bind_layout_handle layout, u32 count, u32 set, u32 binding_t, u8 shader_stage_visibility);
		void						  bind_layout_add_descriptor(gfx_bind_layout_handle layout, u8 type, u32 set, u32 binding_t, u8 shader_stage_visibility);
		void						  bind_layout_add_pointer(gfx_bind_layout_handle layout, const vector_t<bind_layout_pointer_param_t>& pointer_params, u8 shader_stage_visibility);
		void						  bind_layout_add_immutable_sampler(gfx_bind_layout_handle layout, u32 set, u32 binding_t, const sampler_desc_t& desc, u8 shader_stage_visibility);
		void						  finalize_bind_layout(gfx_bind_layout_handle id, bool is_compute, bool is_dyn_index, const char* name);
		void						  bind_group_update_constants(gfx_bind_group_handle group, u8 binding_index, u8* constants, u8 count);
		void						  bind_group_update_descriptor(gfx_bind_group_handle group, u8 binding_index, gfx_resource_handle resource_t);
		void						  bind_group_update_pointer(gfx_bind_group_handle group, u8 binding_index, const bind_group_pointer_t* updates, u16 update_count);
		void						  bind_group_update_pointer(gfx_bind_group_handle group, u8 binding_index, const vector_t<bind_group_pointer_t>& updates);

		void destroy_resource(gfx_resource_handle id);
		void destroy_texture(gfx_texture_handle id);
		void destroy_sampler(gfx_sampler_handle id);
		void destroy_swapchain(gfx_swapchain_handle id);
		void destroy_semaphore(gfx_semaphore_handle id);
		void destroy_shader(gfx_shader_handle id);
		void destroy_bind_group(gfx_bind_group_handle id);
		void destroy_command_buffer(gfx_command_buffer_handle id);
		void destroy_command_allocator(gfx_command_allocator_handle id);
		void destroy_queue(gfx_queue_handle id);
		void destroy_bind_layout(gfx_bind_layout_handle id);

		void wait_semaphore(gfx_semaphore_handle id, u64 value) const;
		void map_resource(gfx_resource_handle id, u8*& ptr) const;
		void unmap_resource(gfx_resource_handle id) const;

		inline HANDLE get_swapchain_latency_handle(gfx_swapchain_handle id)
		{
			return _swapchains.get(id).frame_latency_waitable;
		}

		HANDLE get_shared_handle_for_texture(gfx_texture_handle id);

		static u32	 get_texture_size(u32 width, u32 height, u32 bpp);
		static u32	 align_texture_size(u32 size);
		static u32	 align_texture_size_pitch(u32 size);
		static void* adjust_buffer_pitch(void* data, u32 width, u32 height, u8 bpp, u32& out_total_size);

		void cmd_begin_event(gfx_command_buffer_handle cmd_list, const char* label);
		void cmd_end_event(gfx_command_buffer_handle cmd_list);
		void cmd_begin_render_pass(gfx_command_buffer_handle cmd_list, const command_begin_render_pass_t& command);
		void cmd_begin_render_pass_depth(gfx_command_buffer_handle cmd_list, const command_begin_render_pass_depth_t& command);
		void cmd_begin_render_pass_depth_read_only(gfx_command_buffer_handle cmd_list, const command_begin_render_pass_depth_t& command);
		void cmd_begin_render_pass_depth_only(gfx_command_buffer_handle cmd_list, const command_begin_render_pass_depth_only_t& command);
		void cmd_begin_render_pass_swapchain(gfx_command_buffer_handle cmd_list, const command_begin_render_pass_swapchain_t& command);
		void cmd_begin_render_pass_swapchain_depth(gfx_command_buffer_handle cmd_list, const command_begin_render_pass_swapchain_depth_t& command);
		void cmd_end_render_pass(gfx_command_buffer_handle cmd_list, const command_end_render_pass_t& command) const;
		void cmd_set_scissors(gfx_command_buffer_handle cmd_list, const command_set_scissors_t& command) const;
		void cmd_set_viewport(gfx_command_buffer_handle cmd_list, const command_set_viewport_t& command) const;
		void cmd_bind_pipeline(gfx_command_buffer_handle cmd_list, const command_bind_pipeline_t& command) const;
		void cmd_bind_pipeline_compute(gfx_command_buffer_handle cmd_list, const command_bind_pipeline_compute_t& command) const;
		void cmd_draw_instanced(gfx_command_buffer_handle cmd_list, const command_draw_instanced_t& command) const;
		void cmd_draw_indexed_instanced(gfx_command_buffer_handle cmd_list, const command_draw_indexed_instanced_t& command) const;
		void cmd_execute_indirect(gfx_command_buffer_handle cmd_list, const command_draw_indirect_t& command) const;
		void cmd_bind_vertex_buffers(gfx_command_buffer_handle cmd_list, const command_bind_vertex_buffers_t& command) const;
		void cmd_bind_index_buffers(gfx_command_buffer_handle cmd_list, const command_bind_index_buffers_t& command) const;
		void cmd_copy_resource(gfx_command_buffer_handle cmd_list, const command_copy_resource_t& command) const;
		void cmd_copy_resource_region(gfx_command_buffer_handle cmd_list, const command_copy_resource_region_t& command) const;
		void cmd_copy_buffer_to_texture(gfx_command_buffer_handle cmd_list, const command_copy_buffer_to_texture_t& command);
		void cmd_copy_buffer_region_to_texture(gfx_command_buffer_handle cmd_list, const command_copy_buffer_region_to_texture_t& command) const;
		void cmd_copy_texture_to_buffer(gfx_command_buffer_handle cmd_list, const command_copy_texture_to_buffer_t& command) const;
		void cmd_copy_texture_to_texture(gfx_command_buffer_handle cmd_list, const command_copy_texture_to_texture_t& command) const;
		void cmd_bind_constants(gfx_command_buffer_handle cmd_list, const command_bind_constants_t& command) const;
		void cmd_bind_constants_compute(gfx_command_buffer_handle cmd_list, const command_bind_constants_t& command) const;
		void cmd_bind_layout(gfx_command_buffer_handle cmd_list, const command_bind_layout_t& command) const;
		void cmd_bind_layout_compute(gfx_command_buffer_handle cmd_list, const command_bind_layout_compute_t& command) const;
		void cmd_bind_group(gfx_command_buffer_handle cmd_list, const command_bind_group_t& command) const;
		void cmd_bind_group_compute(gfx_command_buffer_handle cmd_list, const command_bind_group_t& command) const;
		void cmd_dispatch(gfx_command_buffer_handle cmd_list, const command_dispatch_t& command) const;
		void cmd_barrier(gfx_command_buffer_handle cmd_list, const command_barrier_t& command);

		inline gfx_queue_handle get_queue_gfx() const
		{
			return _queue_graphics;
		}

		inline gfx_queue_handle get_queue_transfer() const
		{
			return _queue_transfer;
		}

		inline gfx_queue_handle get_queue_compute() const
		{
			return _queue_compute;
		}

		inline ID3D12Device* get_device() const
		{
			return _device.Get();
		}

		inline ID3D12GraphicsCommandList4* get_gfx_cmd_list(gfx_command_buffer_handle list)
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
		dynamic_gen_pool_t<resource_t, gfx_id_t, gfx_resource_handle_tag>					  _resources;
		dynamic_gen_pool_t<texture_t, gfx_id_t, gfx_texture_handle_tag>						  _textures;
		dynamic_gen_pool_t<texture_shared_handle_t, gfx_id_t, gfx_texture_shared_handle_tag>  _texture_shared_handles;
		dynamic_gen_pool_t<sampler_t, gfx_id_t, gfx_sampler_handle_tag>						  _samplers;
		dynamic_gen_pool_t<swapchain_t, gfx_id_t, gfx_swapchain_handle_tag>					  _swapchains;
		dynamic_gen_pool_t<semaphore_t, gfx_id_t, gfx_semaphore_handle_tag>					  _semaphores;
		dynamic_gen_pool_t<shader_t, gfx_id_t, gfx_shader_handle_tag>						  _shaders;
		dynamic_gen_pool_t<bind_group_t, gfx_id_t, gfx_bind_group_handle_tag>				  _bind_groups;
		dynamic_gen_pool_t<command_buffer_t, gfx_id_t, gfx_command_buffer_handle_tag>		  _command_buffers;
		dynamic_gen_pool_t<command_allocator_t, gfx_id_t, gfx_command_allocator_handle_tag>	  _command_allocators;
		dynamic_gen_pool_t<queue_t, gfx_id_t, gfx_queue_handle_tag>							  _queues;
		dynamic_gen_pool_t<indirect_signature_t, gfx_id_t, gfx_indirect_signature_handle_tag> _indirect_signatures;
		dynamic_gen_pool_t<descriptor_handle_t, gfx_id_t, gfx_descriptor_handle_tag>		  _descriptors;
		dynamic_gen_pool_t<bind_layout_t, gfx_id_t, gfx_bind_layout_handle_tag>				  _bind_layouts;

		dx12_heap_t _heap_rtv		  = {};
		dx12_heap_t _heap_dsv		  = {};
		dx12_heap_t _heap_gpu_buffer  = {};
		dx12_heap_t _heap_gpu_sampler = {};

		D3D12MA::Allocator*						   _allocator = nullptr;
		Microsoft::WRL::ComPtr<IDXGIAdapter1>	   _adapter	  = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Device>	   _device	  = nullptr;
		Microsoft::WRL::ComPtr<IDXGIFactory4>	   _factory	  = nullptr;
		static Microsoft::WRL::ComPtr<IDxcLibrary> s_idxcLib;

		static bool ensure_idxc_lib();

		gfx_queue_handle _queue_graphics	= {};
		gfx_queue_handle _queue_transfer	= {};
		gfx_queue_handle _queue_compute		= {};
		bool			 _tearing_supported = false;

		vector_t<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_dest_descriptors_buffer  = {};
		vector_t<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_dest_descriptors_sampler = {};
		vector_t<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_src_descriptors_buffer	  = {};
		vector_t<D3D12_CPU_DESCRIPTOR_HANDLE> _reuse_src_descriptors_sampler  = {};
		vector_t<CD3DX12_ROOT_PARAMETER1>	  _reuse_root_params			  = {};
		vector_t<CD3DX12_DESCRIPTOR_RANGE1>	  _reuse_root_ranges			  = {};
		vector_t<D3D12_STATIC_SAMPLER_DESC>	  _reuse_static_samplers		  = {};

		friend class app;
		friend class renderer_t;
	};
}
