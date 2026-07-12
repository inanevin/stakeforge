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

#include "paint.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <sfg/math/math.hpp>

namespace sfg::ui
{
	namespace
	{
		resource_handle_t text_pipeline_for(const paint_pipelines_t& pipelines, glyph_raster_mode_e raster_mode)
		{
			switch (raster_mode)
			{
			case glyph_raster_mode_e::lcd:
				return pipelines.text_pipeline;
			case glyph_raster_mode_e::grayscale:
				return pipelines.grayscale_text_pipeline;
			case glyph_raster_mode_e::sdf:
				return pipelines.sdf_pipeline;
			}
			return NULL_RESOURCE_HANDLE;
		}
	}

	void paint_layer_t::init(u32 max_widgets)
	{
		_defs.init(max_widgets);
		_clip_stack.init(max_widgets);
		_defs.resize(max_widgets);
	}

	void paint_layer_t::uninit()
	{
		_clip_stack.uninit();
		_defs.uninit();
	}

	paint_def_t& paint_layer_t::def(widget_id_t id)
	{
		SFG_ASSERT(id < _defs.size());
		return _defs[id];
	}

	const paint_def_t& paint_layer_t::def_const(widget_id_t id) const
	{
		SFG_ASSERT(id < _defs.size());
		return _defs[id];
	}

	void paint_layer_t::clear(widget_id_t id)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id] = {};
	}

	void paint_layer_t::set_rect(widget_id_t id, const vg_rect_paint_t& p, const ui_render_state_t& state)
	{
		SFG_ASSERT(id < _defs.size());
		paint_def_t& def = _defs[id];
		def.kind		 = paint_kind_e::rect;
		def.rect		 = p;
		def.render_state = state;
		if (def.render_state.pipeline == NULL_RESOURCE_HANDLE)
			def.render_state.pipeline = _pipelines.default_pipeline;
	}

	void paint_layer_t::set_text(widget_id_t id, const char* text, u32 len, const vg_text_style_t& s, const ui_render_state_t& state)
	{
		SFG_ASSERT(id < _defs.size());
		paint_def_t& def = _defs[id];
		def.kind		 = paint_kind_e::text;
		def.text		 = s;
		def.text_data	 = text;
		def.text_len	 = len;
		def.render_state = state;
		if (def.render_state.pipeline == NULL_RESOURCE_HANDLE)
			def.render_state.pipeline = text_pipeline_for(_pipelines, s.raster_mode);
	}

	void paint_layer_t::set_text_raster_mode(glyph_raster_mode_e raster_mode)
	{
		for (paint_def_t& def : _defs)
		{
			if (def.kind == paint_kind_e::text)
			{
				def.text.raster_mode	  = raster_mode;
				def.render_state.pipeline = text_pipeline_for(_pipelines, raster_mode);
			}
		}
	}

	void paint_layer_t::set_custom(widget_id_t id, paint_custom_fn fn, void* user_data, const ui_render_state_t& state)
	{
		SFG_ASSERT(id < _defs.size());
		paint_def_t& def = _defs[id];
		def.kind		 = paint_kind_e::custom;
		def.custom_fn	 = fn;
		def.custom_ud	 = user_data;
		def.render_state = state;
		if (def.render_state.pipeline == NULL_RESOURCE_HANDLE)
			def.render_state.pipeline = _pipelines.default_pipeline;
	}

	void paint_layer_t::set_hover_color(widget_id_t id, const vec4f_t& c)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].hover_color = c;
		_defs[id].state_flags |= psf_has_hover;
	}

	void paint_layer_t::set_press_color(widget_id_t id, const vec4f_t& c)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].press_color = c;
		_defs[id].state_flags |= psf_has_press;
	}

	void paint_layer_t::set_focus_color(widget_id_t id, const vec4f_t& c)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].focus_color = c;
		_defs[id].state_flags |= psf_has_focus;
	}

	void paint_layer_t::set_disabled_color(widget_id_t id, const vec4f_t& c)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].disabled_color = c;
		_defs[id].state_flags |= psf_has_disabled;
	}

	void paint_layer_t::set_state_source(widget_id_t id, widget_id_t source)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].state_source = source;
	}

	void paint_layer_t::update_text_layout(layout_tree_t& tree, f32 ui_scale, f32 dpi_scale)
	{
		resource_manager_t& rm = resource_manager_t::get();

		vg_text_paint_t paint = {};
		const f32		scale = get_valid_scale(ui_scale);

		for (u32 i = 0; i < static_cast<u32>(_defs.size()); ++i)
		{
			const paint_def_t& pd = _defs[i];
			if (pd.kind != paint_kind_e::text || pd.text_data == nullptr || pd.text_len == 0)
				continue;

			const font_runtime_t* font = rm.find_runtime<font_runtime_t>(pd.text.font);
			if (font == nullptr || font->face == nullptr)
				continue;

			paint.font		  = font;
			paint.color		  = pd.text.color;
			paint.size_px	  = pd.text.point_size;
			paint.raster_px	  = get_text_raster_px(pd.text.point_size * scale, dpi_scale);
			paint.spacing	  = static_cast<f32>(pd.text.spacing);
			paint.raster_mode = pd.text.raster_mode;
			paint.flip_uv	  = pd.text.flip_uv;

			const vec2f_t m	 = vg_canvas_t::measure_text(pd.text_data, pd.text_len, paint);
			layout_in_t&  in = tree.in(static_cast<widget_id_t>(i));
			in.size_mode_x	 = axis_mode_e::fixed;
			in.size_mode_y	 = axis_mode_e::fixed;
			in.size_value.x	 = m.x;
			in.size_value.y	 = m.y;
		}
	}

	void paint_layer_t::paint_all(const layout_tree_t& tree, const input_router_t& input, vg_canvas_t& canvas, f32 ui_scale, f32 dpi_scale)
	{
		const auto dfs			   = tree.get_dfs();
		const auto dfs_descendants = tree.get_dfs_descendants();
		const f32  scale		   = get_valid_scale(ui_scale);

		_clip_stack.resize(0);

		const widget_id_t get_hovered	  = input.get_hovered();
		const widget_id_t get_focused	  = input.get_focused();
		const widget_id_t pressed_l		  = input.is_pressed(mouse_button_e::left);
		const auto		  push_child_clip = [&](const layout_in_t& in, const layout_out_t& o, u8 depth) {
			const vec4f_t margins = {
				in.child_margins.x * scale,
				in.child_margins.y * scale,
				in.child_margins.z * scale,
				in.child_margins.w * scale,
			};
			const vec4f_t clip = {
				o.pos.x + margins.w,
				o.pos.y + margins.x,
				math::max(0.0f, o.size.x - margins.w - margins.y),
				math::max(0.0f, o.size.y - margins.x - margins.z),
			};
			canvas.push_clip(clip, in.child_clip_mode);
			_clip_stack.push_back({in.child_clip_mode, depth});
		};

		for (size_t i = 0; i < dfs.size; ++i)
		{
			const widget_id_t id = dfs.data[i];
			const u8		  d	 = tree.node(id).depth;

			while (!_clip_stack.empty() && _clip_stack.back().depth >= d)
			{
				const clip_mode_e mode = _clip_stack.back().mode;
				_clip_stack.pop_back();
				canvas.pop_clip(mode);
			}

			const layout_in_t&	in = tree.in_const(id);
			const layout_out_t& o  = tree.out(id);

			if ((in.flags & wf_visible) == 0)
			{
				i += dfs_descendants.data[i];
				continue;
			}

			if (id == tree.get_root())
			{
				if (in.child_clip_mode != clip_mode_e::none)
					push_child_clip(in, o, d);
				continue;
			}

			if (o.clip.z <= 0.0f || o.clip.w <= 0.0f)
				continue;

			const paint_def_t& pd		  = _defs[id];
			const u32		   draw_order = tree.draw_order_const(id);
			const widget_id_t  state_id	  = pd.state_source != NULL_WIDGET ? pd.state_source : id;
			const layout_in_t& state_in	  = tree.in_const(state_id);

			vec4f_t override_color = {0, 0, 0, 0};
			bool	has_override   = false;
			if ((pd.state_flags & psf_has_disabled) && (state_in.flags & wf_disabled))
			{
				override_color = pd.disabled_color;
				has_override   = true;
			}
			else if ((pd.state_flags & psf_has_press) && pressed_l == state_id)
			{
				override_color = pd.press_color;
				has_override   = true;
			}
			else if ((pd.state_flags & psf_has_hover) && get_hovered == state_id)
			{
				override_color = pd.hover_color;
				has_override   = true;
			}

			if (pd.kind == paint_kind_e::rect)
			{
				vg_rect_paint_t paint = pd.rect;
				if (has_override)
				{
					paint.fill_color_a = override_color;
					paint.fill_color_b = override_color;
					paint.gradient	   = vg_gradient_e::none;
				}
				if ((pd.state_flags & psf_has_focus) && get_focused == state_id)
				{
					paint.outline_color = pd.focus_color;
					if (paint.outline_thickness <= 0.0f)
						paint.outline_thickness = 1.0f;
				}
				canvas.add_rect({o.pos.x, o.pos.y}, {o.pos.x + o.size.x, o.pos.y + o.size.y}, paint, pd.render_state, draw_order);
			}
			else if (pd.kind == paint_kind_e::text && pd.text_data != nullptr && pd.text_len > 0)
			{
				resource_manager_t&	  rm   = resource_manager_t::get();
				const font_runtime_t* font = rm.find_runtime<font_runtime_t>(pd.text.font);
				if (font != nullptr && font->face != nullptr)
				{
					vg_text_paint_t paint = {};
					paint.font			  = font;
					paint.color			  = has_override ? override_color : pd.text.color;
					paint.size_px		  = pd.text.point_size * scale;
					paint.raster_px		  = get_text_raster_px(paint.size_px, dpi_scale);
					paint.spacing		  = static_cast<f32>(pd.text.spacing) * scale;
					paint.raster_mode	  = pd.text.raster_mode;
					paint.flip_uv		  = pd.text.flip_uv;

					canvas.add_text(pd.text_data, pd.text_len, {o.pos.x, o.pos.y}, paint, pd.render_state, draw_order);
				}
			}
			else if (pd.kind == paint_kind_e::custom && pd.custom_fn)
			{
				pd.custom_fn(*this, id, canvas, pd.custom_ud);
			}

			if (in.child_clip_mode != clip_mode_e::none)
				push_child_clip(in, o, d);
		}

		while (!_clip_stack.empty())
		{
			const clip_mode_e mode = _clip_stack.back().mode;
			_clip_stack.pop_back();
			canvas.pop_clip(mode);
		}
	}
}
