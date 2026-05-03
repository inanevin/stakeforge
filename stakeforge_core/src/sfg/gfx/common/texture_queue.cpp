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
		_transits.reserve(reserve_count);
	}

	void texture_queue_t::uninit()
	{
		for (entry_t& e : _uploads)
			release_entry(e);
		_uploads.resize(0);
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
		entry.texture	  = desc.texture;
		entry.staging	  = desc.staging;
		entry.from_states = desc.from_states;
		entry.to_states	  = desc.to_states;
		entry.ownership	  = desc.ownership;
		for (size_t i = 0; i < desc.mips.size; i++)
			entry.mips.push_back(desc.mips.data[i]);

		_uploads.push_back(std::move(entry));
	}

	void texture_queue_t::flush(gfx_command_buffer_handle cmd)
	{
		if (_uploads.empty())
			return;

		gfx_backend& backend = gfx_backend::get();

		for (entry_t& e : _uploads)
		{
			if (e.from_states != 0)
			{
				const barrier_t pre = {
					.from_states = e.from_states,
					.to_states	 = resource_state_copy_dest,
					.texture_t	 = e.texture,
					.flags		 = barrier_flags::baf_is_texture,
				};
				backend.cmd_barrier(cmd, {.barriers = &pre, .barrier_count = 1});
			}

			command_copy_buffer_to_texture_t cp = {};
			cp.textures							= e.mips.data();
			cp.destination_texture				= e.texture;
			cp.intermediate_buffer				= e.staging;
			cp.mip_levels						= static_cast<u8>(e.mips.size());
			cp.destination_slice				= 0;
			backend.cmd_copy_buffer_to_texture(cmd, cp);

			if (e.to_states != 0)
				_transits.push_back({.texture = e.texture, .to_states = e.to_states});

			release_entry(e);
		}

		_uploads.resize(0);
	}

	void texture_queue_t::transit(gfx_command_buffer_handle cmd)
	{
		if (_transits.empty())
			return;

		gfx_backend& backend = gfx_backend::get();

		for (const transit_entry_t& te : _transits)
		{
			const barrier_t b = {
				.from_states = resource_state_copy_dest,
				.to_states	 = te.to_states,
				.texture_t	 = te.texture,
				.flags		 = barrier_flags::baf_is_texture,
			};
			backend.cmd_barrier(cmd, {.barriers = &b, .barrier_count = 1});
		}
		_transits.resize(0);
	}
}
