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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/static_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>

namespace sfg
{
	enum class texture_data_ownership_e : u8
	{
		none,		  // queue does not free pixel data
		c_free,		  // SFG_FREE on each mip's pixels
		delete_array, // delete[] on each mip's pixels
	};

	struct texture_upload_desc_t
	{
		gfx_texture_handle			   texture	   = {};
		gfx_resource_handle			   staging	   = {};
		span_t<const texture_buffer_t> mips		   = {};
		u32							   from_states = 0;
		u32							   to_states   = 0;
		texture_data_ownership_e	   ownership   = texture_data_ownership_e::none;
	};

	class texture_queue_t
	{
	public:
		static constexpr u32 MAX_MIPS = 16;

		texture_queue_t()								   = default;
		texture_queue_t(const texture_queue_t&)			   = delete;
		texture_queue_t& operator=(const texture_queue_t&) = delete;
		~texture_queue_t();

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 reserve_count = 16);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void add(const texture_upload_desc_t& desc);
		void flush(gfx_command_buffer_handle cmd);
		void transit(gfx_command_buffer_handle cmd);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bool has_uploads() const
		{
			return !_uploads.empty();
		}
		inline bool has_transits() const
		{
			return !_transits.empty();
		}

	private:
		struct entry_t
		{
			gfx_texture_handle							texture		= {};
			gfx_resource_handle							staging		= {};
			static_vector_t<texture_buffer_t, MAX_MIPS> mips		= {};
			u32											from_states = 0;
			u32											to_states	= 0;
			texture_data_ownership_e					ownership	= texture_data_ownership_e::none;
		};

		struct transit_entry_t
		{
			gfx_texture_handle texture	 = {};
			u32				   to_states = 0;
		};

		static void release_entry(entry_t& entry);

	private:
		vector_t<entry_t>		  _uploads	= {};
		vector_t<transit_entry_t> _transits = {};
	};
}
