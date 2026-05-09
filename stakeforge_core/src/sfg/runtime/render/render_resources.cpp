// Copyright (c) 2025 Inan Evin

#include "render_resources.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>

namespace sfg
{
	render_resources_t& render_resources_t::get()
	{
		static render_resources_t instance;
		return instance;
	}

	void render_resources_t::enqueue_create_resource(sid_t hash, resource_type_e type, const resource_desc_t& desc)
	{
		_create_resource_q.enqueue({.hash = hash, .type = type, .desc = desc});
	}

	void render_resources_t::enqueue_create_texture(sid_t hash, const texture_desc_t& desc)
	{
		_create_texture_q.enqueue({.hash = hash, .desc = desc});
	}

	void render_resources_t::enqueue_create_sampler(sid_t hash, resource_type_e type, const sampler_desc_t& desc)
	{
		_create_sampler_q.enqueue({.hash = hash, .type = type, .desc = desc});
	}

	void render_resources_t::enqueue_create_shader(sid_t hash, resource_type_e type, u32 user_data, const shader_desc_t& desc, vector_t<shader_blob_t>&& blobs, gfx_bind_layout_handle existing_layout, span_t<u8> layout_data)
	{
		create_shader_request_t req = {};
		req.hash					= hash;
		req.type					= type;
		req.user_data				= user_data;
		req.desc					= desc;
		req.blobs					= std::move(blobs);
		req.existing_layout			= existing_layout;
		if (layout_data.data != nullptr && layout_data.size != 0)
		{
			req.layout_data.resize(layout_data.size);
			SFG_MEMCPY(req.layout_data.data(), layout_data.data, layout_data.size);
		}
		_create_shader_q.enqueue(std::move(req));
	}

	void render_resources_t::enqueue_destroy_resource(gfx_resource_handle handle)
	{
		if (handle.is_null())
			return;
		_destroy_resource_q.enqueue(handle);
	}

	void render_resources_t::enqueue_destroy_texture(gfx_texture_handle handle)
	{
		if (handle.is_null())
			return;
		_destroy_texture_q.enqueue(handle);
	}

	void render_resources_t::enqueue_destroy_sampler(gfx_sampler_handle handle)
	{
		if (handle.is_null())
			return;
		_destroy_sampler_q.enqueue(handle);
	}

	void render_resources_t::enqueue_destroy_shader(gfx_shader_handle handle)
	{
		if (handle.is_null())
			return;
		_destroy_shader_q.enqueue(handle);
	}

	bool render_resources_t::drain_completion(render_resource_completion_t& out_completion)
	{
		return _completed_q.try_dequeue(out_completion);
	}

	void render_resources_t::flush_create_destroys()
	{
		SFG_ASSERT(SFG_IS_RENDER_THREAD() || !SFG_IS_RENDER_RUNNING());

		gfx_backend& backend = gfx_backend::get();

		gfx_resource_handle resource_to_destroy = {};
		while (_destroy_resource_q.try_dequeue(resource_to_destroy))
			backend.destroy_resource(resource_to_destroy);

		gfx_texture_handle to_destroy = {};
		while (_destroy_texture_q.try_dequeue(to_destroy))
			backend.destroy_texture(to_destroy);

		gfx_sampler_handle sampler_to_destroy = {};
		while (_destroy_sampler_q.try_dequeue(sampler_to_destroy))
			backend.destroy_sampler(sampler_to_destroy);

		gfx_shader_handle shader_to_destroy = {};
		while (_destroy_shader_q.try_dequeue(shader_to_destroy))
			backend.destroy_shader(shader_to_destroy);

		create_resource_request_t resource_req = {};
		while (_create_resource_q.try_dequeue(resource_req))
		{
			const gfx_resource_handle handle = backend.create_resource(resource_req.desc);
			_completed_q.enqueue({
				.hash	  = resource_req.hash,
				.type	  = resource_req.type,
				.kind	  = render_resource_kind_e::resource,
				.state	  = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.resource = handle,
			});
		}

		create_texture_request_t req = {};
		while (_create_texture_q.try_dequeue(req))
		{
			const gfx_texture_handle handle = backend.create_texture(req.desc);
			_completed_q.enqueue({
				.hash	 = req.hash,
				.type	 = resource_type_e::texture,
				.kind	 = render_resource_kind_e::texture,
				.state	 = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.texture = handle,
			});
		}

		create_sampler_request_t sampler_req = {};
		while (_create_sampler_q.try_dequeue(sampler_req))
		{
			const gfx_sampler_handle handle = backend.create_sampler(sampler_req.desc);
			_completed_q.enqueue({
				.hash	 = sampler_req.hash,
				.type	 = sampler_req.type,
				.kind	 = render_resource_kind_e::sampler,
				.state	 = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.sampler = handle,
			});
		}

		create_shader_request_t shader_req = {};
		while (_create_shader_q.try_dequeue(shader_req))
		{
			const span_t<u8>		layout_data = {.data = shader_req.layout_data.empty() ? nullptr : shader_req.layout_data.data(), .size = shader_req.layout_data.size()};
			const gfx_shader_handle handle		= backend.create_shader(shader_req.desc, shader_req.blobs, shader_req.existing_layout, layout_data);
			_completed_q.enqueue({
				.hash	   = shader_req.hash,
				.type	   = shader_req.type,
				.kind	   = render_resource_kind_e::shader,
				.state	   = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.user_data = shader_req.user_data,
				.shader	   = handle,
			});
		}
	}
}
