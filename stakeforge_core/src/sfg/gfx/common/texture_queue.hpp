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
		none,
		c_free,
		delete_array,
	};

	struct texture_upload_desc_t
	{
		gfx_texture_handle			   texture			 = {};
		gfx_resource_handle			   staging			 = {};
		span_t<const texture_buffer_t> mips				 = {};
		u32							   target_states	 = 0;
		u8							   destination_slice = 0;
		texture_data_ownership_e	   ownership		 = texture_data_ownership_e::none;
	};

	struct texture_region_upload_desc_t
	{
		gfx_texture_handle	src_texture	  = {};
		gfx_texture_handle	dst_texture	  = {};
		gfx_resource_handle src_buffer	  = {};
		u64					src_offset	  = 0;
		u32					src_row_pitch = 0;
		u16					dst_x		  = 0;
		u16					dst_y		  = 0;
		u16					width		  = 0;
		u16					height		  = 0;
		u8					bpp			  = 0;
		u8					dst_mip		  = 0;
		u32					target_states = 0;
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

		void init(u32 reserve_count = 32);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void add(const texture_upload_desc_t& desc);
		void add_region(const texture_region_upload_desc_t& desc);

		bool prepare(gfx_command_buffer_handle cmd);

		// flush issues copies on the (transfer) command buffer; returns true if anything was emitted.
		bool flush(gfx_command_buffer_handle cmd);

		// transit issues post-upload barriers on the (graphics) command buffer.
		void transit(gfx_command_buffer_handle cmd);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		bool has_uploads() const;
		bool has_transits() const;

	private:
		struct entry_t
		{
			gfx_texture_handle							texture			  = {};
			gfx_resource_handle							staging			  = {};
			static_vector_t<texture_buffer_t, MAX_MIPS> mips			  = {};
			u32											target_states	  = 0;
			u8											destination_slice = 0;
			texture_data_ownership_e					ownership		  = texture_data_ownership_e::none;
		};

		struct region_entry_t
		{
			gfx_texture_handle	dst_texture	  = {};
			gfx_resource_handle src_buffer	  = {};
			u64					src_offset	  = 0;
			u32					src_row_pitch = 0;
			u16					dst_x		  = 0;
			u16					dst_y		  = 0;
			u16					width		  = 0;
			u16					height		  = 0;
			u8					bpp			  = 0;
			u8					dst_mip		  = 0;
			u32					target_states = 0;
		};

		struct transit_entry_t
		{
			gfx_texture_handle texture		 = {};
			u32				   target_states = 0;
		};

		static void release_entry(entry_t& entry);

	private:
		vector_t<entry_t>		  _uploads	= {};
		vector_t<region_entry_t>  _regions	= {};
		vector_t<transit_entry_t> _transits = {};
	};
}
