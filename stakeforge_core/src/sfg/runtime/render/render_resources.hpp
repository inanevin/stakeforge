// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/texture_queue.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/vendor/moodycamel/readerwriterqueue.h>

namespace sfg
{
	struct render_resource_t
	{
	};

	struct render_texture_upload_desc_t
	{
		span_t<const texture_buffer_t> mips				 = {};
		render_resource_handle_t	   texture			 = {};
		render_resource_handle_t	   staging			 = {};
		u32							   target_states	 = 0;
		u8							   destination_slice = 0;
		texture_data_ownership_e	   ownership		 = texture_data_ownership_e::none;
	};

	struct render_texture_region_upload_desc_t
	{
		u64						 src_offset	   = 0;
		render_resource_handle_t dst_texture   = {};
		render_resource_handle_t src_buffer	   = {};
		u32						 src_row_pitch = 0;
		u32						 target_states = 0;
		u16						 dst_x		   = 0;
		u16						 dst_y		   = 0;
		u16						 width		   = 0;
		u16						 height		   = 0;
		u8						 bpp		   = 0;
		u8						 dst_mip	   = 0;
	};

	struct render_data_upload_desc_t
	{
		const void*				 data		= nullptr;
		render_resource_handle_t resource	= {};
		u64						 dst_offset = 0;
		u32						 data_size	= 0;
	};

	class render_resources_t
	{
	public:
		static render_resources_t& get();

		render_resources_t()									 = default;
		~render_resources_t()									 = default;
		render_resources_t(const render_resources_t&)			 = delete;
		render_resources_t& operator=(const render_resources_t&) = delete;

		// -----------------------------------------------------------------------------
		// creation
		// -----------------------------------------------------------------------------

		render_resource_handle_t enqueue_create_resource(sid_t hash, resource_type_e type, const resource_desc_t& desc, u32 user_data = 0);
		render_resource_handle_t enqueue_create_texture(sid_t hash, const texture_desc_t& desc, resource_type_e type = resource_type_e::texture, u32 user_data = 0);
		render_resource_handle_t enqueue_create_sampler(sid_t hash, resource_type_e type, const sampler_desc_t& desc);
		render_resource_handle_t enqueue_create_shader(sid_t hash, resource_type_e type, u32 user_data, const shader_desc_t& desc, span_t<const shader_blob_t> blobs, gfx_handle_t existing_layout = {});

		// -----------------------------------------------------------------------------
		// destroy
		// -----------------------------------------------------------------------------

		void enqueue_destroy_resource(render_resource_handle_t handle);
		void enqueue_destroy_texture(render_resource_handle_t handle);
		void enqueue_destroy_sampler(render_resource_handle_t handle);
		void enqueue_destroy_shader(render_resource_handle_t handle);

		// -----------------------------------------------------------------------------
		// upload
		// -----------------------------------------------------------------------------

		void enqueue_texture_upload(const render_texture_upload_desc_t& desc);
		void enqueue_texture_region_upload(const render_texture_region_upload_desc_t& desc);
		void enqueue_data_upload(const render_data_upload_desc_t& desc);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void drain_requests();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		gfx_handle_t get_resource(render_resource_handle_t handle) const;
		gfx_handle_t get_texture(render_resource_handle_t handle) const;
		gfx_handle_t get_sampler_hw(render_resource_handle_t handle) const;
		gfx_handle_t get_shader_hw(render_resource_handle_t handle) const;
		gpu_index_t	 get_resource_gpu_index(render_resource_handle_t handle) const;
		gpu_index_t	 get_texture_gpu_index(render_resource_handle_t handle, u8 view_index) const;
		gpu_index_t	 get_sampler_gpu_index(render_resource_handle_t handle) const;

		inline texture_queue_t& get_texture_upload_queue()
		{
			return _texture_upload_queue;
		}

	private:
		static constexpr u8 MAX_SHADER_STAGES = 4;

		enum class request_kind_e : u8
		{
			create_resource,
			create_texture,
			create_sampler,
			create_shader,
			texture_upload,
			texture_region_upload,
			data_upload,
			destroy_resource,
			destroy_texture,
			destroy_sampler,
			destroy_shader,
		};

		struct request_t
		{
			request_kind_e												  kind				= request_kind_e::create_resource;
			resource_desc_t												  resource_desc		= {};
			texture_desc_t												  texture_desc		= {};
			sampler_desc_t												  sampler_desc		= {};
			shader_desc_t												  shader_desc		= {};
			inplace_vector_t<shader_blob_t, MAX_SHADER_STAGES>			  blobs				= {};
			gfx_handle_t												  existing_layout	= {};
			render_resource_handle_t									  render_handle		= {};
			render_resource_handle_t									  texture			= {};
			render_resource_handle_t									  staging			= {};
			render_resource_handle_t									  dst_texture		= {};
			render_resource_handle_t									  src_buffer		= {};
			render_resource_handle_t									  resource			= {};
			inplace_vector_t<texture_buffer_t, texture_queue_t::MAX_MIPS> mips				= {};
			u8*															  data				= nullptr;
			u64															  src_offset		= 0;
			u64															  dst_offset		= 0;
			u32															  src_row_pitch		= 0;
			u32															  target_states		= 0;
			u32															  data_size			= 0;
			u16															  dst_x				= 0;
			u16															  dst_y				= 0;
			u16															  width				= 0;
			u16															  height			= 0;
			u8															  bpp				= 0;
			u8															  dst_mip			= 0;
			u8															  destination_slice = 0;
			texture_data_ownership_e									  ownership			= texture_data_ownership_e::none;
		};

		struct render_thread_resource_t
		{
			gpu_index_t				 texture_gpu_indices[texture_desc_t::MAX_VIEWS] = {};
			render_resource_handle_t render_handle									= {};
			gfx_handle_t			 hw_handle										= {};
			gpu_index_t				 gpu_index										= NULL_GPU_INDEX;
		};

		static void							   set_render_thread_resource(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle, gfx_handle_t hw_handle, gpu_index_t gpu_index = NULL_GPU_INDEX);
		static void							   set_render_thread_texture(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle, gfx_handle_t hw_handle, const texture_desc_t& desc);
		static const render_thread_resource_t& get_render_thread_resource_entry(const vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle);
		static gfx_handle_t					   remove_render_thread_resource(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle);

		moodycamel::ReaderWriterQueue<request_t> _request_q;
		texture_queue_t							 _texture_upload_queue = {};

		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _resources;
		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _textures;
		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _samplers;
		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _shaders;

		vector_t<render_thread_resource_t> _rt_resources;
		vector_t<render_thread_resource_t> _rt_textures;
		vector_t<render_thread_resource_t> _rt_samplers;
		vector_t<render_thread_resource_t> _rt_shaders;
	};
}
