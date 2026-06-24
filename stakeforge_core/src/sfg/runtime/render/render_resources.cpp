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

	render_resource_handle_t render_resources_t::enqueue_create_resource(sid_t, resource_type_e, const resource_desc_t& desc, u32)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		const render_resource_handle_t handle = _resources.add();
		_request_q.enqueue({.kind = request_kind_e::create_resource, .resource_desc = desc, .render_handle = handle});
		return handle;
	}

	render_resource_handle_t render_resources_t::enqueue_create_texture(sid_t, const texture_desc_t& desc, resource_type_e, u32)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		const render_resource_handle_t handle = _textures.add();
		_request_q.enqueue({.kind = request_kind_e::create_texture, .texture_desc = desc, .render_handle = handle});
		return handle;
	}

	render_resource_handle_t render_resources_t::enqueue_create_sampler(sid_t, resource_type_e, const sampler_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		const render_resource_handle_t handle = _samplers.add();
		_request_q.enqueue({.kind = request_kind_e::create_sampler, .sampler_desc = desc, .render_handle = handle});
		return handle;
	}

	render_resource_handle_t render_resources_t::enqueue_create_shader(sid_t, resource_type_e, u32, const shader_desc_t& desc, span_t<const shader_blob_t> blobs, gfx_handle_t existing_layout)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(blobs.size <= MAX_SHADER_STAGES);
		const render_resource_handle_t handle = _shaders.add();
		request_t					   req	  = {};
		req.kind							  = request_kind_e::create_shader;
		req.shader_desc						  = desc;
		req.existing_layout					  = existing_layout;
		req.render_handle					  = handle;
		for (size_t i = 0; i < blobs.size; ++i)
		{
			const shader_blob_t& blob = blobs.data[i];
			u8*					 copy = static_cast<u8*>(SFG_MALLOC(blob.data.size));
			SFG_MEMCPY(copy, blob.data.data, blob.data.size);
			req.blobs.push_back({
				.stage = blob.stage,
				.data  = {.data = copy, .size = blob.data.size},
			});
		}
		_request_q.enqueue(std::move(req));
		return handle;
	}

	void render_resources_t::enqueue_destroy_resource(render_resource_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_resource, .render_handle = handle});
		_resources.remove(handle);
	}

	void render_resources_t::enqueue_destroy_texture(render_resource_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_texture, .render_handle = handle});
		_textures.remove(handle);
	}

	void render_resources_t::enqueue_destroy_sampler(render_resource_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_sampler, .render_handle = handle});
		_samplers.remove(handle);
	}

	void render_resources_t::enqueue_destroy_shader(render_resource_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_shader, .render_handle = handle});
		_shaders.remove(handle);
	}

	void render_resources_t::enqueue_texture_upload(const render_texture_upload_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(!desc.texture.is_null());
		SFG_ASSERT(!desc.staging.is_null());
		SFG_ASSERT(desc.mips.size > 0);
		SFG_ASSERT(desc.mips.size <= texture_queue_t::MAX_MIPS);

		request_t req		  = {};
		req.kind			  = request_kind_e::texture_upload;
		req.texture			  = desc.texture;
		req.staging			  = desc.staging;
		req.target_states	  = desc.target_states;
		req.destination_slice = desc.destination_slice;
		req.ownership		  = desc.ownership;
		for (size_t i = 0; i < desc.mips.size; ++i)
			req.mips.push_back(desc.mips.data[i]);

		_request_q.enqueue(std::move(req));
	}

	void render_resources_t::enqueue_texture_region_upload(const render_texture_region_upload_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(!desc.dst_texture.is_null());
		SFG_ASSERT(!desc.src_buffer.is_null());
		SFG_ASSERT(desc.width > 0 && desc.height > 0);
		SFG_ASSERT(desc.bpp > 0);
		_request_q.enqueue({
			.kind		   = request_kind_e::texture_region_upload,
			.dst_texture   = desc.dst_texture,
			.src_buffer	   = desc.src_buffer,
			.src_offset	   = desc.src_offset,
			.src_row_pitch = desc.src_row_pitch,
			.target_states = desc.target_states,
			.dst_x		   = desc.dst_x,
			.dst_y		   = desc.dst_y,
			.width		   = desc.width,
			.height		   = desc.height,
			.bpp		   = desc.bpp,
			.dst_mip	   = desc.dst_mip,
		});
	}

	void render_resources_t::enqueue_data_upload(const render_data_upload_desc_t& desc)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(!desc.resource.is_null());
		SFG_ASSERT(desc.data != nullptr);
		SFG_ASSERT(desc.data_size != 0);

		u8* copy = static_cast<u8*>(SFG_MALLOC(desc.data_size));
		SFG_MEMCPY(copy, desc.data, desc.data_size);
		_request_q.enqueue({.kind = request_kind_e::data_upload, .resource = desc.resource, .data = copy, .dst_offset = desc.dst_offset, .data_size = desc.data_size});
	}

	gfx_handle_t render_resources_t::get_resource(render_resource_handle_t handle) const
	{
		SFG_ASSERT(is_render_access_thread());
		return get_render_thread_resource_entry(_rt_resources, handle).hw_handle;
	}

	gfx_handle_t render_resources_t::get_texture(render_resource_handle_t handle) const
	{
		SFG_ASSERT(is_render_access_thread());
		return get_render_thread_resource_entry(_rt_textures, handle).hw_handle;
	}

	gfx_handle_t render_resources_t::get_sampler_hw(render_resource_handle_t handle) const
	{
		SFG_ASSERT(is_render_access_thread());
		return get_render_thread_resource_entry(_rt_samplers, handle).hw_handle;
	}

	gfx_handle_t render_resources_t::get_shader_hw(render_resource_handle_t handle) const
	{
		SFG_ASSERT(is_render_access_thread());
		return get_render_thread_resource_entry(_rt_shaders, handle).hw_handle;
	}

	gpu_index_t render_resources_t::get_resource_gpu_index(render_resource_handle_t handle) const
	{
		SFG_ASSERT(is_render_access_thread());
		return get_render_thread_resource_entry(_rt_resources, handle).gpu_index;
	}

	gpu_index_t render_resources_t::get_texture_gpu_index(render_resource_handle_t handle, u8 view_index) const
	{
		SFG_ASSERT(is_render_access_thread());
		SFG_ASSERT(view_index < texture_desc_t::MAX_VIEWS);
		const render_thread_resource_t& resource = get_render_thread_resource_entry(_rt_textures, handle);
		const gpu_index_t				idx		 = resource.texture_gpu_indices[view_index];
		SFG_ASSERT(idx != NULL_GPU_INDEX);
		return idx;
	}

	gpu_index_t render_resources_t::get_sampler_gpu_index(render_resource_handle_t handle) const
	{
		SFG_ASSERT(is_render_access_thread());
		return get_render_thread_resource_entry(_rt_samplers, handle).gpu_index;
	}

	void render_resources_t::drain_requests()
	{
		SFG_ASSERT(is_render_access_thread());

		gfx_backend& backend = gfx_backend::get();

		request_t req = {};
		while (_request_q.try_dequeue(req))
		{
			switch (req.kind)
			{
			case request_kind_e::create_resource: {
				const gfx_handle_t handle = backend.create_resource(req.resource_desc);
				set_render_thread_resource(_rt_resources, req.render_handle, handle, backend.get_resource_gpu_index(handle));
				break;
			}
			case request_kind_e::create_texture: {
				const gfx_handle_t handle = backend.create_texture(req.texture_desc);
				set_render_thread_texture(_rt_textures, req.render_handle, handle, req.texture_desc);
				break;
			}
			case request_kind_e::create_sampler: {
				const gfx_handle_t handle = backend.create_sampler(req.sampler_desc);
				set_render_thread_resource(_rt_samplers, req.render_handle, handle, backend.get_sampler_gpu_index(handle));
				break;
			}
			case request_kind_e::create_shader: {
				const span_t<const shader_blob_t> blobs	 = {.data = req.blobs.data(), .size = req.blobs.size()};
				const gfx_handle_t				  handle = backend.create_shader(req.shader_desc, blobs, req.existing_layout);
				for (shader_blob_t& blob : req.blobs)
					SFG_FREE(blob.data.data);
				set_render_thread_resource(_rt_shaders, req.render_handle, handle);
				break;
			}
			case request_kind_e::texture_upload: {
				const texture_upload_desc_t desc = {
					.texture		   = get_render_thread_resource_entry(_rt_textures, req.texture).hw_handle,
					.staging		   = get_render_thread_resource_entry(_rt_resources, req.staging).hw_handle,
					.mips			   = {.data = req.mips.data(), .size = req.mips.size()},
					.target_states	   = req.target_states,
					.destination_slice = req.destination_slice,
					.ownership		   = req.ownership,
				};
				_texture_upload_queue.add(desc);
				break;
			}
			case request_kind_e::texture_region_upload: {
				const texture_region_upload_desc_t desc = {
					.dst_texture   = get_render_thread_resource_entry(_rt_textures, req.dst_texture).hw_handle,
					.src_buffer	   = get_render_thread_resource_entry(_rt_resources, req.src_buffer).hw_handle,
					.src_offset	   = req.src_offset,
					.src_row_pitch = req.src_row_pitch,
					.dst_x		   = req.dst_x,
					.dst_y		   = req.dst_y,
					.width		   = req.width,
					.height		   = req.height,
					.bpp		   = req.bpp,
					.dst_mip	   = req.dst_mip,
					.target_states = req.target_states,
				};
				_texture_upload_queue.add_region(desc);
				break;
			}
			case request_kind_e::data_upload: {
				u8*				   mapped	= nullptr;
				const gfx_handle_t resource = get_render_thread_resource_entry(_rt_resources, req.resource).hw_handle;
				backend.map_resource(resource, mapped);
				SFG_MEMCPY(mapped + req.dst_offset, req.data, req.data_size);
				backend.unmap_resource(resource);
				SFG_FREE(req.data);
				break;
			}
			case request_kind_e::destroy_resource: {
				const gfx_handle_t handle = remove_render_thread_resource(_rt_resources, req.render_handle);
				if (!handle.is_null())
					backend.destroy_resource(handle);
				break;
			}
			case request_kind_e::destroy_texture: {
				const gfx_handle_t handle = remove_render_thread_resource(_rt_textures, req.render_handle);
				if (!handle.is_null())
					backend.destroy_texture(handle);
				break;
			}
			case request_kind_e::destroy_sampler: {
				const gfx_handle_t handle = remove_render_thread_resource(_rt_samplers, req.render_handle);
				if (!handle.is_null())
					backend.destroy_sampler(handle);
				break;
			}
			case request_kind_e::destroy_shader: {
				const gfx_handle_t handle = remove_render_thread_resource(_rt_shaders, req.render_handle);
				if (!handle.is_null())
					backend.destroy_shader(handle);
				break;
			}
			}
		}
	}

	void render_resources_t::set_render_thread_resource(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle, gfx_handle_t hw_handle, gpu_index_t gpu_index)
	{
		if (resources.size() <= render_handle.index)
			resources.resize(static_cast<size_t>(render_handle.index) + 1);

		render_thread_resource_t& resource = resources[render_handle.index];
		resource						   = {};
		for (gpu_index_t& idx : resource.texture_gpu_indices)
			idx = NULL_GPU_INDEX;
		resource.render_handle = render_handle;
		resource.hw_handle	   = hw_handle;
		resource.gpu_index	   = gpu_index;
	}

	void render_resources_t::set_render_thread_texture(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle, gfx_handle_t hw_handle, const texture_desc_t& desc)
	{
		set_render_thread_resource(resources, render_handle, hw_handle);

		render_thread_resource_t& resource = resources[render_handle.index];
		SFG_ASSERT(desc.view_count <= texture_desc_t::MAX_VIEWS);
		for (u8 i = 0; i < desc.view_count; ++i)
			resource.texture_gpu_indices[i] = gfx_backend::get().get_texture_gpu_index(hw_handle, i);
		resource.gpu_index = resource.texture_gpu_indices[0];
	}

	const render_resources_t::render_thread_resource_t& render_resources_t::get_render_thread_resource_entry(const vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle)
	{
		SFG_ASSERT(render_handle.index < resources.size());
		const render_thread_resource_t& resource = resources[render_handle.index];
		SFG_ASSERT(resource.render_handle == render_handle);
		SFG_ASSERT(!resource.hw_handle.is_null());
		return resource;
	}

	gfx_handle_t render_resources_t::remove_render_thread_resource(vector_t<render_thread_resource_t>& resources, render_resource_handle_t render_handle)
	{
		SFG_ASSERT(render_handle.index < resources.size());
		render_thread_resource_t& resource = resources[render_handle.index];
		SFG_ASSERT(resource.render_handle == render_handle);
		const gfx_handle_t hw_handle = resource.hw_handle;
		resource					 = {};
		return hw_handle;
	}

}
