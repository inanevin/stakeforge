// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/data/static_vector.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/texture_queue.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/vendor/moodycamel/readerwriterqueue.h>

namespace sfg
{
	enum class render_resource_kind_e : u8
	{
		resource,
		texture,
		sampler,
		shader,
	};

	struct render_resource_completion_t
	{
		sid_t				   hash		 = 0;
		resource_type_e		   type		 = resource_type_e::invalid;
		render_resource_kind_e kind		 = render_resource_kind_e::resource;
		resource_state_e	   state	 = resource_state_e::failed;
		u32					   user_data = 0;
		gfx_resource_handle	   resource	 = {};
		gfx_texture_handle	   texture	 = {};
		gfx_sampler_handle	   sampler	 = {};
		gfx_shader_handle	   shader	 = {};
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

		void enqueue_create_resource(sid_t hash, resource_type_e type, const resource_desc_t& desc);
		void enqueue_create_texture(sid_t hash, const texture_desc_t& desc);
		void enqueue_create_sampler(sid_t hash, resource_type_e type, const sampler_desc_t& desc);
		void enqueue_create_shader(sid_t hash, resource_type_e type, u32 user_data, const shader_desc_t& desc, span_t<const shader_blob_t> blobs, gfx_bind_layout_handle existing_layout = {});

		// -----------------------------------------------------------------------------
		// destroy
		// -----------------------------------------------------------------------------

		void enqueue_destroy_resource(gfx_resource_handle handle);
		void enqueue_destroy_texture(gfx_texture_handle handle);
		void enqueue_destroy_sampler(gfx_sampler_handle handle);
		void enqueue_destroy_shader(gfx_shader_handle handle);

		// -----------------------------------------------------------------------------
		// upload
		// -----------------------------------------------------------------------------

		void enqueue_texture_upload(const texture_upload_desc_t& desc);
		void enqueue_texture_region_upload(const texture_region_upload_desc_t& desc);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool drain_completion(render_resource_completion_t& out_completion);
		void drain_requests();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline texture_queue_t& get_texture_upload_queue()
		{
			return _texture_upload_queue;
		}

	private:
		static constexpr u8 MAX_SHADER_STAGES = 4;

		struct create_resource_request_t
		{
			sid_t			hash = 0;
			resource_type_e type = resource_type_e::invalid;
			resource_desc_t desc = {};
		};

		struct create_texture_request_t
		{
			sid_t		   hash = 0;
			texture_desc_t desc = {};
		};

		struct create_sampler_request_t
		{
			sid_t			hash = 0;
			resource_type_e type = resource_type_e::invalid;
			sampler_desc_t	desc = {};
		};

		struct create_shader_request_t
		{
			sid_t											  hash			  = 0;
			resource_type_e									  type			  = resource_type_e::invalid;
			u32												  user_data		  = 0;
			shader_desc_t									  desc			  = {};
			static_vector_t<shader_blob_t, MAX_SHADER_STAGES> blobs			  = {};
			gfx_bind_layout_handle							  existing_layout = {};
		};

		struct texture_upload_request_t
		{
			gfx_texture_handle											 texture	   = {};
			gfx_resource_handle											 staging	   = {};
			static_vector_t<texture_buffer_t, texture_queue_t::MAX_MIPS> mips		   = {};
			u32															 target_states = 0;
			texture_data_ownership_e									 ownership	   = texture_data_ownership_e::none;
		};

		moodycamel::ReaderWriterQueue<create_resource_request_t>	_create_resource_q;
		moodycamel::ReaderWriterQueue<create_texture_request_t>		_create_texture_q;
		moodycamel::ReaderWriterQueue<create_sampler_request_t>		_create_sampler_q;
		moodycamel::ReaderWriterQueue<create_shader_request_t>		_create_shader_q;
		moodycamel::ReaderWriterQueue<texture_upload_request_t>		_texture_upload_q;
		moodycamel::ReaderWriterQueue<texture_region_upload_desc_t> _texture_region_upload_q;
		moodycamel::ReaderWriterQueue<gfx_resource_handle>			_destroy_resource_q;
		moodycamel::ReaderWriterQueue<gfx_texture_handle>			_destroy_texture_q;
		moodycamel::ReaderWriterQueue<gfx_sampler_handle>			_destroy_sampler_q;
		moodycamel::ReaderWriterQueue<gfx_shader_handle>			_destroy_shader_q;
		moodycamel::ReaderWriterQueue<render_resource_completion_t> _completed_q;
		texture_queue_t												_texture_upload_queue = {};
	};
}
