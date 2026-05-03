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
#include <sfg/data/vector.hpp>
#include <sfg/ui/ui_common.hpp>

namespace sfg::ui
{
	class vg_atlas_t
	{
	public:
		vg_atlas_t()							 = default;
		vg_atlas_t(const vg_atlas_t&)			 = delete;
		vg_atlas_t& operator=(const vg_atlas_t&) = delete;
		~vg_atlas_t();

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 width, u32 height, bool is_lcd);
		void uninit();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline u32 get_width() const
		{
			return _width;
		}
		inline u32 get_height() const
		{
			return _height;
		}
		inline u8* get_data() const
		{
			return _data;
		}
		inline u32 get_data_size() const
		{
			return _data_size;
		}
		inline bool get_is_lcd() const
		{
			return _is_lcd;
		}
		inline u32 get_id() const
		{
			return _id;
		}
		inline void set_id(u32 i)
		{
			_id = i;
		}
		inline bool is_dirty() const
		{
			return _dirty;
		}
		inline void mark_dirty()
		{
			_dirty = true;
		}
		inline void clear_dirty()
		{
			_dirty = false;
		}

	private:
		u8*	 _data		= nullptr;
		u32	 _data_size = 0;
		u32	 _width		= 0;
		u32	 _height	= 0;
		u32	 _id		= invalid_id_u32;
		bool _is_lcd	= false;
		bool _dirty		= false;
	};
}
