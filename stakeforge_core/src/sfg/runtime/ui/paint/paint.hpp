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
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class layout_tree_t;

	struct paint_pipelines_t
	{
		resource_handle_t default_pipeline		  = 0;
		resource_handle_t text_pipeline			  = 0;
		resource_handle_t grayscale_text_pipeline = 0;
		resource_handle_t sdf_pipeline			  = 0;
	};

	enum class paint_kind_e : u8
	{
		none,
		rect,
		text,
		custom,
	};

	enum paint_state_flag_e : u8
	{
		psf_has_hover	 = 1 << 0,
		psf_has_press	 = 1 << 1,
		psf_has_focus	 = 1 << 2,
		psf_has_disabled = 1 << 3,
	};

	class vg_canvas_t;

	using paint_custom_fn = void (*)(paint_layer_t& paint, widget_id_t id, vg_canvas_t& canvas, void* user_data);

	struct paint_def_t
	{
		paint_kind_e	  kind		   = paint_kind_e::none;
		u8				  state_flags  = 0;
		vg_rect_paint_t	  rect		   = {};
		vg_text_style_t	  text		   = {};
		ui_render_state_t render_state = {};
		// paint layer does not own this memory.
		const char* text_data = nullptr;
		u32			text_len  = 0;

		vec4f_t		hover_color	   = {0, 0, 0, 0};
		vec4f_t		press_color	   = {0, 0, 0, 0};
		vec4f_t		focus_color	   = {0, 0, 0, 0};
		vec4f_t		disabled_color = {0, 0, 0, 0};
		widget_id_t state_source   = NULL_WIDGET;

		paint_custom_fn custom_fn = nullptr;
		void*			custom_ud = nullptr;
	};

	class paint_layer_t
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 max_widgets);
		void uninit();
		void update_text_layout(layout_tree_t& tree, f32 ui_scale, f32 dpi_scale);
		void paint_all(const layout_tree_t& tree, const input_router_t& input, vg_canvas_t& canvas, f32 ui_scale, f32 dpi_scale);

		// -----------------------------------------------------------------------------
		// widget
		// -----------------------------------------------------------------------------

		paint_def_t&	   def(widget_id_t id);
		const paint_def_t& def_const(widget_id_t id) const;
		void			   clear(widget_id_t id);

		// -----------------------------------------------------------------------------
		// draw
		// -----------------------------------------------------------------------------

		void set_rect(widget_id_t id, const vg_rect_paint_t& p, const ui_render_state_t& state = {});
		void set_text(widget_id_t id, const char* text, u32 len, const vg_text_style_t& s, const ui_render_state_t& state = {});
		void set_text_raster_mode(glyph_raster_mode_e raster_mode);
		void set_custom(widget_id_t id, paint_custom_fn fn, void* user_data, const ui_render_state_t& state = {});
		void set_hover_color(widget_id_t id, const vec4f_t& c);
		void set_press_color(widget_id_t id, const vec4f_t& c);
		void set_focus_color(widget_id_t id, const vec4f_t& c);
		void set_disabled_color(widget_id_t id, const vec4f_t& c);
		void set_state_source(widget_id_t id, widget_id_t source);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_pipelines(const paint_pipelines_t& pipelines)
		{
			_pipelines = pipelines;
		}

		inline const paint_pipelines_t& get_pipelines() const
		{
			return _pipelines;
		}

	private:
		vector_t<paint_def_t> _defs;
		vector_t<u8>		  _clip_stack;
		paint_pipelines_t	  _pipelines = {};
	};
}
