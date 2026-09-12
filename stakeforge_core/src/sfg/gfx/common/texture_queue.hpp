/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/semaphore_data.hpp>
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
		gfx_handle_t				   texture			 = {};
		gfx_handle_t				   staging			 = {};
		span_t<const texture_buffer_t> mips				 = {};
		u32							   target_states	 = 0;
		u8							   destination_slice = 0;
		texture_data_ownership_e	   ownership		 = texture_data_ownership_e::none;
	};

	struct texture_region_upload_desc_t
	{
		gfx_handle_t src_texture   = {};
		gfx_handle_t dst_texture   = {};
		gfx_handle_t src_buffer	   = {};
		u64			 src_offset	   = 0;
		u32			 src_row_pitch = 0;
		u16			 dst_x		   = 0;
		u16			 dst_y		   = 0;
		u16			 width		   = 0;
		u16			 height		   = 0;
		u8			 bpp		   = 0;
		u8			 dst_mip	   = 0;
		u32			 target_states = 0;
	};

	struct texture_queue_submit_desc_t
	{
		gfx_handle_t	  queue_gfx		 = {};
		gfx_handle_t	  queue_transfer = {};
		gfx_handle_t	  cmd_prepare	 = {};
		gfx_handle_t	  cmd_transfer	 = {};
		gfx_handle_t	  cmd_transit	 = {};
		semaphore_data_t* semaphore		 = nullptr;
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

		bool prepare(gfx_handle_t cmd);

		// flush issues copies on the (transfer) command buffer; returns true if anything was emitted.
		bool flush(gfx_handle_t cmd);

		// transit issues post-upload barriers on the (graphics) command buffer.
		void transit(gfx_handle_t cmd);

		bool submit(const texture_queue_submit_desc_t& desc);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		bool has_uploads() const;
		bool has_transits() const;

	private:
		struct entry_t
		{
			gfx_handle_t								 texture		   = {};
			gfx_handle_t								 staging		   = {};
			inplace_vector_t<texture_buffer_t, MAX_MIPS> mips			   = {};
			u32											 target_states	   = 0;
			u8											 destination_slice = 0;
			texture_data_ownership_e					 ownership		   = texture_data_ownership_e::none;
		};

		struct region_entry_t
		{
			gfx_handle_t dst_texture   = {};
			gfx_handle_t src_buffer	   = {};
			u64			 src_offset	   = 0;
			u32			 src_row_pitch = 0;
			u16			 dst_x		   = 0;
			u16			 dst_y		   = 0;
			u16			 width		   = 0;
			u16			 height		   = 0;
			u8			 bpp		   = 0;
			u8			 dst_mip	   = 0;
			u32			 target_states = 0;
		};

		struct transit_entry_t
		{
			gfx_handle_t texture	   = {};
			u32			 target_states = 0;
		};

		static void release_entry(entry_t& entry);

	private:
		vector_t<entry_t>		  _uploads	= {};
		vector_t<region_entry_t>  _regions	= {};
		vector_t<transit_entry_t> _transits = {};
	};
}
