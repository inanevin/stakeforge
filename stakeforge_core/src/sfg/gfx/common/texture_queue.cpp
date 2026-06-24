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

#include "texture_queue.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/barrier_description.hpp>
#include <sfg/gfx/common/commands.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	texture_queue_t::~texture_queue_t() = default;

	void texture_queue_t::init(u32 reserve_count)
	{
		_uploads.reserve(reserve_count);
		_regions.reserve(reserve_count);
		_transits.reserve(reserve_count);
	}

	void texture_queue_t::uninit()
	{
		for (entry_t& e : _uploads)
			release_entry(e);
		_uploads.resize(0);
		_regions.resize(0);
		_transits.resize(0);
	}

	void texture_queue_t::release_entry(entry_t& entry)
	{
		if (entry.ownership == texture_data_ownership_e::none)
			return;

		for (texture_buffer_t& mip : entry.mips)
		{
			if (mip.pixels == nullptr)
				continue;

			if (entry.ownership == texture_data_ownership_e::c_free)
				SFG_FREE(mip.pixels);
			else
				delete[] mip.pixels;

			mip.pixels = nullptr;
		}
	}

	void texture_queue_t::add(const texture_upload_desc_t& desc)
	{
		SFG_ASSERT(!desc.texture.is_null());
		SFG_ASSERT(!desc.staging.is_null());
		SFG_ASSERT(desc.mips.size > 0);
		SFG_ASSERT(desc.mips.size <= MAX_MIPS);

		entry_t entry;
		entry.texture			= desc.texture;
		entry.staging			= desc.staging;
		entry.target_states		= desc.target_states;
		entry.destination_slice = desc.destination_slice;
		entry.ownership			= desc.ownership;
		for (size_t i = 0; i < desc.mips.size; i++)
			entry.mips.push_back(desc.mips.data[i]);

		_uploads.push_back(std::move(entry));
	}

	void texture_queue_t::add_region(const texture_region_upload_desc_t& desc)
	{
		SFG_ASSERT(!desc.dst_texture.is_null());
		SFG_ASSERT(!desc.src_buffer.is_null());
		SFG_ASSERT(desc.width > 0 && desc.height > 0);
		SFG_ASSERT(desc.bpp > 0);

		region_entry_t r;
		r.dst_texture	= desc.dst_texture;
		r.src_buffer	= desc.src_buffer;
		r.src_offset	= desc.src_offset;
		r.src_row_pitch = desc.src_row_pitch;
		r.dst_x			= desc.dst_x;
		r.dst_y			= desc.dst_y;
		r.width			= desc.width;
		r.height		= desc.height;
		r.bpp			= desc.bpp;
		r.dst_mip		= desc.dst_mip;
		r.target_states = desc.target_states;

		_regions.push_back(r);
	}

	bool texture_queue_t::prepare(gfx_handle_t cmd)
	{
		if (_uploads.empty() && _regions.empty())
			return false;

		gfx_backend& backend = gfx_backend::get();

		bool emitted = false;
		for (const entry_t& e : _uploads)
		{
			const u32 current_states = backend.get_texture_state(e.texture);
			if (current_states != resource_state_common)
			{
				const barrier_t b = {
					.from_states = current_states,
					.to_states	 = resource_state_common,
					.texture_t	 = e.texture,
					.flags		 = barrier_flags::baf_is_texture,
				};
				backend.cmd_barrier(cmd, {.barriers = &b, .barrier_count = 1});
				emitted = true;
			}
		}

		for (const region_entry_t& r : _regions)
		{
			const u32 current_states = backend.get_texture_state(r.dst_texture);
			if (current_states != resource_state_common)
			{
				const barrier_t b = {
					.from_states = current_states,
					.to_states	 = resource_state_common,
					.texture_t	 = r.dst_texture,
					.flags		 = barrier_flags::baf_is_texture,
				};
				backend.cmd_barrier(cmd, {.barriers = &b, .barrier_count = 1});
				emitted = true;
			}
		}

		return emitted;
	}

	bool texture_queue_t::flush(gfx_handle_t cmd)
	{
		if (_uploads.empty() && _regions.empty())
			return false;

		gfx_backend& backend = gfx_backend::get();

		for (entry_t& e : _uploads)
		{
			SFG_ASSERT(backend.get_texture_state(e.texture) == resource_state_common);

			command_copy_buffer_to_texture_t cp = {};
			cp.textures							= e.mips.data();
			cp.destination_texture				= e.texture;
			cp.intermediate_buffer				= e.staging;
			cp.mip_levels						= static_cast<u8>(e.mips.size());
			cp.destination_slice				= e.destination_slice;
			backend.cmd_copy_buffer_to_texture(cmd, cp);

			if (e.target_states != 0)
				_transits.push_back({.texture = e.texture, .target_states = e.target_states});

			release_entry(e);
		}

		for (const region_entry_t& r : _regions)
		{
			SFG_ASSERT(backend.get_texture_state(r.dst_texture) == resource_state_common);

			command_copy_buffer_region_to_texture_t cp = {};
			cp.src_buffer							   = r.src_buffer;
			cp.dst_texture							   = r.dst_texture;
			cp.src_offset							   = r.src_offset;
			cp.src_row_pitch						   = r.src_row_pitch;
			cp.dst_x								   = r.dst_x;
			cp.dst_y								   = r.dst_y;
			cp.width								   = r.width;
			cp.height								   = r.height;
			cp.dst_mip								   = r.dst_mip;
			cp.bpp									   = r.bpp;
			backend.cmd_copy_buffer_region_to_texture(cmd, cp);

			if (r.target_states != 0)
				_transits.push_back({.texture = r.dst_texture, .target_states = r.target_states});
		}

		_uploads.resize(0);
		_regions.resize(0);
		return true;
	}

	void texture_queue_t::transit(gfx_handle_t cmd)
	{
		if (_transits.empty())
			return;

		gfx_backend& backend = gfx_backend::get();

		for (const transit_entry_t& te : _transits)
		{
			const u32 current_states = backend.get_texture_state(te.texture);
			if (current_states != te.target_states)
			{
				const barrier_t b = {
					.from_states = current_states,
					.to_states	 = te.target_states,
					.texture_t	 = te.texture,
					.flags		 = barrier_flags::baf_is_texture,
				};
				backend.cmd_barrier(cmd, {.barriers = &b, .barrier_count = 1});
			}
		}
		_transits.resize(0);
	}

	bool texture_queue_t::has_uploads() const
	{
		return !_uploads.empty() || !_regions.empty();
	}

	bool texture_queue_t::has_transits() const
	{
		return !_transits.empty();
	}
}
