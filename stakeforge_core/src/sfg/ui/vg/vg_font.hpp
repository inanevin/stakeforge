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

#include "common/size_definitions.hpp"
#include "data/unique.hpp"
#include "data/vector.hpp"
#include "ui/ui_common.hpp"

namespace sfg::ui
{
	class vg_atlas_t;

	enum class vg_font_kind_e : u8
	{
		bitmap,
		sdf,
		lcd,
	};

	struct vg_glyph_t
	{
		u8* sdf_data		  = nullptr;
		i32 kern_advance[128] = {0};
		i32 width			  = 0;
		i32 height			  = 0;
		i32 advance_x		  = 0;
		i32 left_bearing	  = 0;
		f32 x_offset		  = 0.0f;
		f32 y_offset		  = 0.0f;
		i32 atlas_x			  = 0;
		i32 atlas_y			  = 0;
		f32 uv_x			  = 0.0f;
		f32 uv_y			  = 0.0f;
		f32 uv_w			  = 0.0f;
		f32 uv_h			  = 0.0f;
	};

	struct vg_font_t
	{
		vg_glyph_t	   glyph_info[128]		  = {};
		vg_atlas_t*	   _atlas				  = nullptr;
		u32			   _font_id				  = invalid_id_u32;
		u32			   _atlas_required_height = 0;
		u32			   _atlas_pos			  = 0;
		f32			   _scale				  = 0.0f;
		i32			   ascent				  = 0;
		i32			   descent				  = 0;
		i32			   line_gap				  = 0;
		u32			   size					  = 0;
		vg_font_kind_e kind					  = vg_font_kind_e::bitmap;
	};

	struct vg_font_config_t
	{
		u32			   size			= 16;
		u32			   range_start	= 32;
		u32			   range_end	= 128;
		vg_font_kind_e kind			= vg_font_kind_e::bitmap;
		i32			   sdf_padding	= 3;
		i32			   sdf_edge		= 128;
		f32			   sdf_distance = 32.0f;
	};

	class vg_font_manager_t
	{
	public:
		vg_font_manager_t();
		vg_font_manager_t(const vg_font_manager_t&)			   = delete;
		vg_font_manager_t& operator=(const vg_font_manager_t&) = delete;
		~vg_font_manager_t();

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 atlas_width, u32 atlas_height);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		vg_font_t* load_font_from_file(const char* path, const vg_font_config_t& cfg);
		vg_font_t* load_font(const u8* data, size_t data_size, const vg_font_config_t& cfg);
		void	   unload_font(vg_font_t* fnt);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const vector_t<unique_t<vg_atlas_t>>& atlases() const
		{
			return _atlases;
		}

	private:
		void place_into_atlas(vg_font_t* fnt);

	private:
		vector_t<unique_t<vg_atlas_t>> _atlases;
		vector_t<unique_t<vg_font_t>>  _fonts;
		u32							   _atlas_width	 = 1024;
		u32							   _atlas_height = 1024;
	};
}
