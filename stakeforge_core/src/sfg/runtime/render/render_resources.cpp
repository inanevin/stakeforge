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

	void render_resources_t::enqueue_create_resource(sid_t hash, resource_type_e type, const resource_desc_t& desc, u32 user_data)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		_create_resource_q.enqueue({.hash = hash, .type = type, .user_data = user_data, .desc = desc});
	}

	void render_resources_t::enqueue_create_texture(sid_t hash, const texture_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		_create_texture_q.enqueue({.hash = hash, .desc = desc});
	}

	void render_resources_t::enqueue_create_sampler(sid_t hash, resource_type_e type, const sampler_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		_create_sampler_q.enqueue({.hash = hash, .type = type, .desc = desc});
	}

	void render_resources_t::enqueue_create_shader(sid_t hash, resource_type_e type, u32 user_data, const shader_desc_t& desc, span_t<const shader_blob_t> blobs, gfx_bind_layout_handle existing_layout)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(blobs.size <= MAX_SHADER_STAGES);
		create_shader_request_t req = {};
		req.hash					= hash;
		req.type					= type;
		req.user_data				= user_data;
		req.desc					= desc;
		req.existing_layout			= existing_layout;
		for (size_t i = 0; i < blobs.size; ++i)
			req.blobs.push_back(blobs.data[i]);
		_create_shader_q.enqueue(std::move(req));
	}

	void render_resources_t::enqueue_destroy_resource(gfx_resource_handle handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_destroy_resource_q.enqueue(handle);
	}

	void render_resources_t::enqueue_destroy_texture(gfx_texture_handle handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_destroy_texture_q.enqueue(handle);
	}

	void render_resources_t::enqueue_destroy_sampler(gfx_sampler_handle handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_destroy_sampler_q.enqueue(handle);
	}

	void render_resources_t::enqueue_destroy_shader(gfx_shader_handle handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_destroy_shader_q.enqueue(handle);
	}

	void render_resources_t::enqueue_texture_upload(const texture_upload_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());

		texture_upload_request_t req = {};
		req.texture					 = desc.texture;
		req.staging					 = desc.staging;
		req.target_states			 = desc.target_states;
		req.ownership				 = desc.ownership;
		for (size_t i = 0; i < desc.mips.size; ++i)
			req.mips.push_back(desc.mips.data[i]);

		_texture_upload_q.enqueue(std::move(req));
	}

	void render_resources_t::enqueue_texture_region_upload(const texture_region_upload_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		_texture_region_upload_q.enqueue(desc);
	}

	void render_resources_t::enqueue_data_upload(gfx_resource_handle resource, const void* data, u32 data_size)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(!resource.is_null());
		SFG_ASSERT(data != nullptr);
		SFG_ASSERT(data_size != 0);

		u8* copy = static_cast<u8*>(SFG_MALLOC(data_size));
		SFG_MEMCPY(copy, data, data_size);
		_data_upload_q.enqueue({.resource = resource, .data = copy, .data_size = data_size});
	}

	bool render_resources_t::drain_completion(render_resource_completion_t& out_completion)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		return _completed_q.try_dequeue(out_completion);
	}

	void render_resources_t::drain_requests()
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
				.hash	   = resource_req.hash,
				.type	   = resource_req.type,
				.kind	   = render_resource_kind_e::resource,
				.state	   = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.user_data = resource_req.user_data,
				.resource  = handle,
				.gpu_index = backend.get_resource_gpu_index(handle),
			});
		}

		create_texture_request_t req = {};
		while (_create_texture_q.try_dequeue(req))
		{
			const gfx_texture_handle handle = backend.create_texture(req.desc);
			_completed_q.enqueue({
				.hash	   = req.hash,
				.type	   = resource_type_e::texture,
				.kind	   = render_resource_kind_e::texture,
				.state	   = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.texture   = handle,
				.gpu_index = backend.get_texture_gpu_index(handle, 0),
			});
		}

		create_sampler_request_t sampler_req = {};
		while (_create_sampler_q.try_dequeue(sampler_req))
		{
			const gfx_sampler_handle handle = backend.create_sampler(sampler_req.desc);
			_completed_q.enqueue({
				.hash	   = sampler_req.hash,
				.type	   = sampler_req.type,
				.kind	   = render_resource_kind_e::sampler,
				.state	   = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.sampler   = handle,
				.gpu_index = backend.get_sampler_gpu_index(handle),
			});
		}

		create_shader_request_t shader_req = {};
		while (_create_shader_q.try_dequeue(shader_req))
		{
			const span_t<const shader_blob_t> blobs	 = {.data = shader_req.blobs.data(), .size = shader_req.blobs.size()};
			const gfx_shader_handle			  handle = backend.create_shader(shader_req.desc, blobs, shader_req.existing_layout);
			_completed_q.enqueue({
				.hash	   = shader_req.hash,
				.type	   = shader_req.type,
				.kind	   = render_resource_kind_e::shader,
				.state	   = handle.is_null() ? resource_state_e::failed : resource_state_e::ready,
				.user_data = shader_req.user_data,
				.shader	   = handle,
			});
		}

		texture_upload_request_t texture_upload_req = {};
		while (_texture_upload_q.try_dequeue(texture_upload_req))
		{
			const texture_upload_desc_t desc = {
				.texture	   = texture_upload_req.texture,
				.staging	   = texture_upload_req.staging,
				.mips		   = {.data = texture_upload_req.mips.data(), .size = texture_upload_req.mips.size()},
				.target_states = texture_upload_req.target_states,
				.ownership	   = texture_upload_req.ownership,
			};
			_texture_upload_queue.add(desc);
		}

		texture_region_upload_desc_t texture_region_upload_req = {};
		while (_texture_region_upload_q.try_dequeue(texture_region_upload_req))
			_texture_upload_queue.add_region(texture_region_upload_req);

		data_upload_request_t data_upload_req = {};
		while (_data_upload_q.try_dequeue(data_upload_req))
		{
			u8* mapped = nullptr;
			backend.map_resource(data_upload_req.resource, mapped);
			SFG_MEMCPY(mapped, data_upload_req.data, data_upload_req.data_size);
			backend.unmap_resource(data_upload_req.resource);
			SFG_FREE(data_upload_req.data);
		}
	}
}
