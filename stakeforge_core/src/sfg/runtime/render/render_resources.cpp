// Copyright (c) 2025 Inan Evin

#include "render_resources.hpp"
#include "render_resources_util.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/util/gfx_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
	render_resources_t& render_resources_t::get()
	{
		static render_resources_t instance;
		return instance;
	}

	void render_resources_t::init(const render_resources_config_t& config)
	{
		get_texture_upload_queue().init(config.texture_upload_initial_capacity);
		_pending_material_parameter_updates.reserve(config.pending_material_update_initial_capacity);

		if (config.resource_initial_capacity != 0)
			_resources.reserve(config.resource_initial_capacity);

		if (config.texture_initial_capacity != 0)
			_textures.reserve(config.texture_initial_capacity);

		if (config.sampler_initial_capacity != 0)
			_samplers.reserve(config.sampler_initial_capacity);

		if (config.shader_initial_capacity != 0)
			_shaders.reserve(config.shader_initial_capacity);

		_default_linear_sampler = enqueue_create_sampler(gfx_util_t::get_sampler_desc_linear());

		const u8 invalid_pixels[] = {0, 0, 0, 255, 255, 0, 255, 255, 255, 0, 255, 255, 0, 0, 0, 255};
		_invalid_texture		  = render_resources_util_t::create_uploaded_texture(*this, format_e::r8g8b8a8_srgb, {2, 2}, invalid_pixels, "invalid_texture", _invalid_texture_staging);

		const u8 white_pixel[] = {255, 255, 255, 255};
		const u8 black_pixel[] = {0, 0, 0, 255};
		_white_texture		   = render_resources_util_t::create_uploaded_texture(*this, format_e::r8g8b8a8_unorm, {1, 1}, white_pixel, "white_texture", _white_texture_staging);
		_black_texture		   = render_resources_util_t::create_uploaded_texture(*this, format_e::r8g8b8a8_unorm, {1, 1}, black_pixel, "black_texture", _black_texture_staging);
		_brdf_lut			   = render_resources_util_t::create_brdf_lut(*this, _brdf_lut_staging);
	}

	void render_resources_t::uninit()
	{
		drain_requests();

		_deferred_destroys.push_back({.kind = request_kind_e::destroy_sampler, .render_handle = _default_linear_sampler});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_texture, .render_handle = _invalid_texture});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_resource, .render_handle = _invalid_texture_staging});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_texture, .render_handle = _white_texture});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_resource, .render_handle = _white_texture_staging});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_texture, .render_handle = _black_texture});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_resource, .render_handle = _black_texture_staging});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_texture, .render_handle = _brdf_lut});
		_deferred_destroys.push_back({.kind = request_kind_e::destroy_resource, .render_handle = _brdf_lut_staging});

		drain_destroy_requests();

		_default_linear_sampler	 = {};
		_invalid_texture		 = {};
		_invalid_texture_staging = {};
		_white_texture			 = {};
		_white_texture_staging	 = {};
		_black_texture			 = {};
		_black_texture_staging	 = {};
		_brdf_lut				 = {};
		_brdf_lut_staging		 = {};

		get_texture_upload_queue().uninit();

		release_retired_resources(true);
		release_retired_textures(true);
		release_retired_samplers(true);
		release_retired_shaders(true);

		_pending_material_parameter_updates.resize(0);
	}

	render_resource_handle_t render_resources_t::enqueue_create_resource(const resource_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		const render_resource_handle_t handle = _resources.add();
		_request_q.enqueue({.kind = request_kind_e::create_resource, .resource_desc = desc, .render_handle = handle});
		return handle;
	}

	render_resource_handle_t render_resources_t::enqueue_create_texture(const texture_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		const render_resource_handle_t handle = _textures.add();
		_request_q.enqueue({.kind = request_kind_e::create_texture, .texture_desc = desc, .render_handle = handle});
		return handle;
	}

	render_resource_handle_t render_resources_t::enqueue_create_sampler(const sampler_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		const render_resource_handle_t handle = _samplers.add();
		_request_q.enqueue({.kind = request_kind_e::create_sampler, .sampler_desc = desc, .render_handle = handle});
		return handle;
	}

	render_resource_handle_t render_resources_t::enqueue_create_shader(const shader_desc_t& desc, span_t<const shader_blob_t> blobs, gfx_handle_t existing_layout)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
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
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_resource, .render_handle = handle});
	}

	void render_resources_t::enqueue_destroy_texture(render_resource_handle_t handle)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_texture, .render_handle = handle});
	}

	void render_resources_t::enqueue_destroy_sampler(render_resource_handle_t handle)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_sampler, .render_handle = handle});
	}

	void render_resources_t::enqueue_destroy_shader(render_resource_handle_t handle)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		if (handle.is_null())
			return;
		_request_q.enqueue({.kind = request_kind_e::destroy_shader, .render_handle = handle});
	}

	void render_resources_t::enqueue_texture_upload(const render_texture_upload_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
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
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
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

	void render_resources_t::enqueue_replace_texture(const render_texture_replace_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(!desc.texture.is_null());
		SFG_ASSERT(!desc.staging.is_null());
		SFG_ASSERT(!desc.old_staging.is_null());
		SFG_ASSERT(desc.mips.size > 0);
		SFG_ASSERT(desc.mips.size <= texture_queue_t::MAX_MIPS);

		request_t req	  = {};
		req.kind		  = request_kind_e::replace_texture;
		req.texture		  = desc.texture;
		req.staging		  = desc.staging;
		req.old_staging	  = desc.old_staging;
		req.texture_desc  = desc.texture_desc;
		req.target_states = desc.target_states;
		req.ownership	  = desc.ownership;
		for (size_t i = 0; i < desc.mips.size; ++i)
			req.mips.push_back(desc.mips.data[i]);

		_resources.remove(desc.old_staging);
		_request_q.enqueue(std::move(req));
	}

	void render_resources_t::enqueue_data_upload(const render_data_upload_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(!desc.resource.is_null());
		SFG_ASSERT(desc.data != nullptr);
		SFG_ASSERT(desc.data_size != 0);

		u8* copy = static_cast<u8*>(SFG_MALLOC(desc.data_size));
		SFG_MEMCPY(copy, desc.data, desc.data_size);
		_request_q.enqueue({.kind = request_kind_e::data_upload, .resource = desc.resource, .data = copy, .dst_offset = desc.dst_offset, .data_size = desc.data_size});
	}

	void render_resources_t::enqueue_material_parameter_update(const render_material_parameter_update_desc_t& desc)
	{
		SFG_ASSERT(is_main_thread() || !SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(desc.material != NULL_SID);
		SFG_ASSERT(desc.data != nullptr);
		SFG_ASSERT(desc.data_size != 0);
		SFG_ASSERT(desc.data_size <= SFG_MATERIAL_MAX_PARAMETER_DATA_SIZE);

		material_parameter_update_t update = {};
		update.material					   = desc.material;
		update.data_size				   = desc.data_size;

		for (u8 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
		{
			SFG_ASSERT(!desc.parameter_buffers[frame_index].is_null());
			update.parameter_buffers[frame_index] = desc.parameter_buffers[frame_index];
		}

		SFG_MEMCPY(update.data, desc.data, desc.data_size);
		_material_parameter_update_q.enqueue(std::move(update));
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
		SFG_ASSERT(view_index < TEXTURE_MAX_VIEWS);
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
		ZoneScoped;

		SFG_ASSERT(is_render_access_thread());

		drain_material_parameter_update_requests();

		gfx_backend& backend = gfx_backend::get();
		release_retired_resources(false);
		release_retired_textures(false);
		release_retired_samplers(false);
		release_retired_shaders(false);

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
			case request_kind_e::replace_texture: {
				const render_thread_resource_t& old_resource = get_render_thread_resource_entry(_rt_textures, req.texture);
				const gfx_handle_t				old_texture	 = old_resource.hw_handle;
				const gfx_handle_t				old_staging	 = remove_render_thread_resource(_rt_resources, req.old_staging);
				const gfx_handle_t				new_texture	 = backend.create_texture(req.texture_desc);
				set_render_thread_texture(_rt_textures, req.texture, new_texture, req.texture_desc);
				_retired_textures.push_back({.texture = old_texture, .frames = BACK_BUFFER_COUNT});
				_retired_resources.push_back({.resource = old_staging, .frames = BACK_BUFFER_COUNT});

				const texture_upload_desc_t desc = {
					.texture		   = new_texture,
					.staging		   = get_render_thread_resource_entry(_rt_resources, req.staging).hw_handle,
					.mips			   = {.data = req.mips.data(), .size = req.mips.size()},
					.target_states	   = req.target_states,
					.destination_slice = 0,
					.ownership		   = req.ownership,
				};
				_texture_upload_queue.add(desc);
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
				_deferred_destroys.push_back(req);
				break;
			}
			case request_kind_e::destroy_texture: {
				_deferred_destroys.push_back(req);
				break;
			}
			case request_kind_e::destroy_sampler: {
				_deferred_destroys.push_back(req);
				break;
			}
			case request_kind_e::destroy_shader: {
				_deferred_destroys.push_back(req);
				break;
			}
			}
		}
	}

	void render_resources_t::flush_material_parameter_updates(u8 frame_index)
	{
		ZoneScoped;

		SFG_ASSERT(is_render_access_thread());
		SFG_ASSERT(frame_index < BACK_BUFFER_COUNT);

		gfx_backend& backend   = gfx_backend::get();
		const u8	 frame_bit = static_cast<u8>(1u << frame_index);

		for (size_t i = 0; i < _pending_material_parameter_updates.size();)
		{
			material_parameter_update_t& update = _pending_material_parameter_updates[i];

			if ((update.dirty_slots & frame_bit) != 0)
			{
				const gfx_handle_t resource = get_render_thread_resource_entry(_rt_resources, update.parameter_buffers[frame_index]).hw_handle;
				u8*				   mapped	= nullptr;

				backend.map_resource(resource, mapped);
				SFG_MEMCPY(mapped, update.data, update.data_size);
				backend.unmap_resource(resource);

				update.dirty_slots &= static_cast<u8>(~frame_bit);
			}

			if (update.dirty_slots == 0)
			{
				update = _pending_material_parameter_updates.back();
				_pending_material_parameter_updates.pop_back();
				continue;
			}

			++i;
		}
	}

	void render_resources_t::drain_destroy_requests()
	{
		ZoneScoped;

		SFG_ASSERT(is_render_access_thread());

		for (const request_t& req : _deferred_destroys)
		{
			switch (req.kind)
			{
			case request_kind_e::destroy_resource: {
				const gfx_handle_t handle = get_render_thread_resource_entry(_rt_resources, req.render_handle).hw_handle;
				_retired_resources.push_back({.render_handle = req.render_handle, .resource = handle, .frames = BACK_BUFFER_COUNT});
				break;
			}
			case request_kind_e::destroy_texture: {
				const gfx_handle_t handle = get_render_thread_resource_entry(_rt_textures, req.render_handle).hw_handle;
				_retired_textures.push_back({.render_handle = req.render_handle, .texture = handle, .frames = BACK_BUFFER_COUNT});
				break;
			}
			case request_kind_e::destroy_sampler: {
				const gfx_handle_t handle = get_render_thread_resource_entry(_rt_samplers, req.render_handle).hw_handle;
				_retired_samplers.push_back({.render_handle = req.render_handle, .sampler = handle, .frames = BACK_BUFFER_COUNT});
				break;
			}
			case request_kind_e::destroy_shader: {
				const gfx_handle_t handle = get_render_thread_resource_entry(_rt_shaders, req.render_handle).hw_handle;
				_retired_shaders.push_back({.render_handle = req.render_handle, .shader = handle, .frames = BACK_BUFFER_COUNT});
				break;
			}
			default:
				break;
			}
		}

		_deferred_destroys.resize(0);
	}

	void render_resources_t::release_retired_resources(bool force)
	{
		gfx_backend& backend = gfx_backend::get();

		for (size_t i = 0; i < _retired_resources.size();)
		{
			retired_resource_t& retired = _retired_resources[i];

			if (force || retired.frames == 0)
			{
				if (!retired.render_handle.is_null())
				{
					const auto pending_end = std::remove_if(_pending_material_parameter_updates.begin(), _pending_material_parameter_updates.end(), [&](const material_parameter_update_t& update) {
						return std::find(std::begin(update.parameter_buffers), std::end(update.parameter_buffers), retired.render_handle) != std::end(update.parameter_buffers);
					});
					_pending_material_parameter_updates.erase(pending_end, _pending_material_parameter_updates.end());

					const gfx_handle_t resource = remove_render_thread_resource(_rt_resources, retired.render_handle);
					SFG_ASSERT(resource == retired.resource);
					_resources.remove(retired.render_handle);
				}

				backend.destroy_resource(retired.resource);
				_retired_resources.erase(_retired_resources.begin() + static_cast<ptrdiff_t>(i));
				continue;
			}

			retired.frames--;
			++i;
		}
	}

	void render_resources_t::drain_material_parameter_update_requests()
	{
		material_parameter_update_t update = {};

		while (_material_parameter_update_q.try_dequeue(update))
		{
			auto it = std::find_if(_pending_material_parameter_updates.begin(), _pending_material_parameter_updates.end(), [&](const material_parameter_update_t& pending) { return pending.material == update.material; });

			if (it == _pending_material_parameter_updates.end())
			{
				_pending_material_parameter_updates.push_back(std::move(update));
				it = _pending_material_parameter_updates.end() - 1;
			}
			else
			{
				it->data_size = update.data_size;

				for (u8 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
					it->parameter_buffers[frame_index] = update.parameter_buffers[frame_index];

				SFG_MEMCPY(it->data, update.data, update.data_size);
			}

			it->dirty_slots = static_cast<u8>((1u << BACK_BUFFER_COUNT) - 1u);
			update			= {};
		}
	}

	void render_resources_t::release_retired_textures(bool force)
	{
		gfx_backend& backend = gfx_backend::get();

		for (size_t i = 0; i < _retired_textures.size();)
		{
			retired_texture_t& retired = _retired_textures[i];

			if (force || retired.frames == 0)
			{
				if (!retired.render_handle.is_null())
				{
					const gfx_handle_t texture = remove_render_thread_resource(_rt_textures, retired.render_handle);
					SFG_ASSERT(texture == retired.texture);
					_textures.remove(retired.render_handle);
				}

				backend.destroy_texture(retired.texture);
				_retired_textures.erase(_retired_textures.begin() + static_cast<ptrdiff_t>(i));
				continue;
			}

			retired.frames--;
			++i;
		}
	}

	void render_resources_t::release_retired_samplers(bool force)
	{
		gfx_backend& backend = gfx_backend::get();

		for (size_t i = 0; i < _retired_samplers.size();)
		{
			retired_sampler_t& retired = _retired_samplers[i];

			if (force || retired.frames == 0)
			{
				if (!retired.render_handle.is_null())
				{
					const gfx_handle_t sampler = remove_render_thread_resource(_rt_samplers, retired.render_handle);
					SFG_ASSERT(sampler == retired.sampler);
					_samplers.remove(retired.render_handle);
				}

				backend.destroy_sampler(retired.sampler);
				_retired_samplers.erase(_retired_samplers.begin() + static_cast<ptrdiff_t>(i));
				continue;
			}

			retired.frames--;
			++i;
		}
	}

	void render_resources_t::release_retired_shaders(bool force)
	{
		gfx_backend& backend = gfx_backend::get();

		for (size_t i = 0; i < _retired_shaders.size();)
		{
			retired_shader_t& retired = _retired_shaders[i];

			if (force || retired.frames == 0)
			{
				if (!retired.render_handle.is_null())
				{
					const gfx_handle_t shader = remove_render_thread_resource(_rt_shaders, retired.render_handle);
					SFG_ASSERT(shader == retired.shader);
					_shaders.remove(retired.render_handle);
				}

				backend.destroy_shader(retired.shader);
				_retired_shaders.erase(_retired_shaders.begin() + static_cast<ptrdiff_t>(i));
				continue;
			}

			retired.frames--;
			++i;
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
		SFG_ASSERT(desc.view_count <= TEXTURE_MAX_VIEWS);
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
