// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/texture_queue.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/resources/material_limits.hpp>
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

	struct render_texture_replace_desc_t
	{
		span_t<const texture_buffer_t> mips			 = {};
		render_resource_handle_t	   texture		 = {};
		render_resource_handle_t	   staging		 = {};
		render_resource_handle_t	   old_staging	 = {};
		texture_desc_t				   texture_desc	 = {};
		u32							   target_states = 0;
		texture_data_ownership_e	   ownership	 = texture_data_ownership_e::none;
	};

	struct render_data_upload_desc_t
	{
		const void*				 data		= nullptr;
		render_resource_handle_t resource	= {};
		u64						 dst_offset = 0;
		u32						 data_size	= 0;
	};

	struct render_material_parameter_update_desc_t
	{
		sid_t					 material							  = NULL_SID;
		const void*				 data								  = nullptr;
		render_resource_handle_t parameter_buffers[BACK_BUFFER_COUNT] = {};
		u16						 data_size							  = 0;
	};

	class render_resources_t
	{
	public:
		static render_resources_t& get();

		render_resources_t()									 = default;
		~render_resources_t()									 = default;
		render_resources_t(const render_resources_t&)			 = delete;
		render_resources_t& operator=(const render_resources_t&) = delete;

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// creation
		// -----------------------------------------------------------------------------

		render_resource_handle_t enqueue_create_resource(const resource_desc_t& desc);
		render_resource_handle_t enqueue_create_texture(const texture_desc_t& desc);
		render_resource_handle_t enqueue_create_sampler(const sampler_desc_t& desc);
		render_resource_handle_t enqueue_create_shader(const shader_desc_t& desc, span_t<const shader_blob_t> blobs, gfx_handle_t existing_layout = {});

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
		void enqueue_replace_texture(const render_texture_replace_desc_t& desc);
		void enqueue_data_upload(const render_data_upload_desc_t& desc);
		void enqueue_material_parameter_update(const render_material_parameter_update_desc_t& desc);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void drain_requests();
		void flush_material_parameter_updates(u8 frame_index);
		void drain_destroy_requests();
		void release_retired_resources(bool force = false);
		void release_retired_textures(bool force = false);
		void release_retired_samplers(bool force = false);
		void release_retired_shaders(bool force = false);

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

		inline render_resource_handle_t get_default_linear_sampler() const
		{
			return _default_linear_sampler;
		}

		inline render_resource_handle_t get_invalid_texture() const
		{
			return _invalid_texture;
		}

		inline render_resource_handle_t get_white_texture() const
		{
			return _white_texture;
		}

		inline render_resource_handle_t get_black_texture() const
		{
			return _black_texture;
		}

		inline render_resource_handle_t get_brdf_lut() const
		{
			return _brdf_lut;
		}

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
			replace_texture,
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
			render_resource_handle_t									  old_staging		= {};
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
			gpu_index_t				 texture_gpu_indices[TEXTURE_MAX_VIEWS] = {};
			render_resource_handle_t render_handle							= {};
			gfx_handle_t			 hw_handle								= {};
			gpu_index_t				 gpu_index								= NULL_GPU_INDEX;
		};

		struct retired_texture_t
		{
			render_resource_handle_t render_handle = {};
			gfx_handle_t			 texture	   = {};
			u8						 frames		   = 0;
		};

		struct retired_resource_t
		{
			render_resource_handle_t render_handle = {};
			gfx_handle_t			 resource	   = {};
			u8						 frames		   = 0;
		};

		struct retired_sampler_t
		{
			render_resource_handle_t render_handle = {};
			gfx_handle_t			 sampler	   = {};
			u8						 frames		   = 0;
		};

		struct retired_shader_t
		{
			render_resource_handle_t render_handle = {};
			gfx_handle_t			 shader		   = {};
			u8						 frames		   = 0;
		};

		struct material_parameter_update_t
		{
			sid_t					 material									= NULL_SID;
			render_resource_handle_t parameter_buffers[BACK_BUFFER_COUNT]		= {};
			u8						 data[SFG_MATERIAL_MAX_PARAMETER_DATA_SIZE] = {};
			u16						 data_size									= 0;
			u8						 dirty_slots								= 0;
		};

		static void							   set_render_thread_resource(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle, gfx_handle_t hw_handle, gpu_index_t gpu_index = NULL_GPU_INDEX);
		static void							   set_render_thread_texture(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle, gfx_handle_t hw_handle, const texture_desc_t& desc);
		static const render_thread_resource_t& get_render_thread_resource_entry(const vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle);
		static gfx_handle_t					   remove_render_thread_resource(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle);
		void								   drain_material_parameter_update_requests();

		moodycamel::ReaderWriterQueue<request_t>				   _request_q;
		moodycamel::ReaderWriterQueue<material_parameter_update_t> _material_parameter_update_q;
		texture_queue_t											   _texture_upload_queue = {};
		vector_t<request_t>										   _deferred_destroys;
		vector_t<material_parameter_update_t>					   _pending_material_parameter_updates;
		render_resource_handle_t								   _default_linear_sampler	= {};
		render_resource_handle_t								   _invalid_texture			= {};
		render_resource_handle_t								   _invalid_texture_staging = {};
		render_resource_handle_t								   _white_texture			= {};
		render_resource_handle_t								   _white_texture_staging	= {};
		render_resource_handle_t								   _black_texture			= {};
		render_resource_handle_t								   _black_texture_staging	= {};
		render_resource_handle_t								   _brdf_lut				= {};
		render_resource_handle_t								   _brdf_lut_staging		= {};

		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _resources;
		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _textures;
		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _samplers;
		dynamic_gen_pool_t<render_resource_t, u32, render_resource_tag_t> _shaders;

		vector_t<render_thread_resource_t> _rt_resources;
		vector_t<render_thread_resource_t> _rt_textures;
		vector_t<render_thread_resource_t> _rt_samplers;
		vector_t<render_thread_resource_t> _rt_shaders;
		vector_t<retired_resource_t>	   _retired_resources;
		vector_t<retired_texture_t>		   _retired_textures;
		vector_t<retired_sampler_t>		   _retired_samplers;
		vector_t<retired_shader_t>		   _retired_shaders;
	};
}
