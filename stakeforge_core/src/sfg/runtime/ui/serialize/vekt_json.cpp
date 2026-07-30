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

#include "vekt_json.hpp"
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg::ui
{
	namespace
	{
		nlohmann::json vec2_to_json(const vec2f_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y});
		}

		nlohmann::json vec4_to_json(const vec4f_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y, value.z, value.w});
		}

		vec2f_t vec2_from_json(const nlohmann::json& j, const vec2f_t& fallback)
		{
			if (!j.is_array() || j.size() < 2)
				return fallback;

			return {j.at(0).get<f32>(), j.at(1).get<f32>()};
		}

		vec4f_t vec4_from_json(const nlohmann::json& j, const vec4f_t& fallback)
		{
			if (!j.is_array() || j.size() < 4)
				return fallback;

			return {j.at(0).get<f32>(), j.at(1).get<f32>(), j.at(2).get<f32>(), j.at(3).get<f32>()};
		}

		nlohmann::json layout_in_to_json(const layout_in_t& in)
		{
			nlohmann::json j;
			j["size"]		 = vec2_to_json(in.size_value);
			j["pos"]		 = vec2_to_json(in.pos_value);
			j["margins"]	 = vec4_to_json(in.child_margins);
			j["spacing"]	 = in.child_spacing;
			j["scroll"]		 = vec2_to_json(in.scroll_offset);
			j["size_mode_x"] = static_cast<u8>(in.size_mode_x);
			j["size_mode_y"] = static_cast<u8>(in.size_mode_y);
			j["pos_mode_x"]	 = static_cast<u8>(in.pos_mode_x);
			j["pos_mode_y"]	 = static_cast<u8>(in.pos_mode_y);
			j["anchor_x"]	 = static_cast<u8>(in.anchor_x);
			j["anchor_y"]	 = static_cast<u8>(in.anchor_y);
			j["flow"]		 = static_cast<u8>(in.flow);
			j["flags"]		 = in.flags;
			j["clip_mode"]	 = static_cast<u8>(in.child_clip_mode);
			return j;
		}

		void layout_in_from_json(const nlohmann::json& j, layout_in_t& in)
		{
			in.size_value	   = vec2_from_json(j.value("size", nlohmann::json::array()), vec2f_t{0, 0});
			in.pos_value	   = vec2_from_json(j.value("pos", nlohmann::json::array()), vec2f_t{0, 0});
			in.child_margins   = vec4_from_json(j.value("margins", nlohmann::json::array()), vec4f_t{0, 0, 0, 0});
			in.child_spacing   = j.value("spacing", 0.0f);
			in.scroll_offset   = vec2_from_json(j.value("scroll", nlohmann::json::array()), vec2f_t{0, 0});
			in.size_mode_x	   = static_cast<axis_mode_e>(j.value("size_mode_x", static_cast<u8>(axis_mode_e::fixed)));
			in.size_mode_y	   = static_cast<axis_mode_e>(j.value("size_mode_y", static_cast<u8>(axis_mode_e::fixed)));
			in.pos_mode_x	   = static_cast<pos_mode_e>(j.value("pos_mode_x", static_cast<u8>(pos_mode_e::flow)));
			in.pos_mode_y	   = static_cast<pos_mode_e>(j.value("pos_mode_y", static_cast<u8>(pos_mode_e::flow)));
			in.anchor_x		   = static_cast<anchor_e>(j.value("anchor_x", static_cast<u8>(anchor_e::start)));
			in.anchor_y		   = static_cast<anchor_e>(j.value("anchor_y", static_cast<u8>(anchor_e::start)));
			in.flow			   = static_cast<flow_e>(j.value("flow", static_cast<u8>(flow_e::none)));
			in.flags		   = j.value("flags", static_cast<u16>(wf_visible));
			in.child_clip_mode = static_cast<clip_mode_e>(j.value("clip_mode", static_cast<u8>(clip_mode_e::none)));
			if ((in.flags & static_cast<u16>(1 << 1)) != 0)
			{
				in.child_clip_mode = clip_mode_e::scissor_rect;
				in.flags &= ~static_cast<u16>(1 << 1);
			}
		}

		nlohmann::json rect_paint_to_json(const vg_rect_paint_t& p)
		{
			nlohmann::json j;
			j["fill_a"]		   = vec4_to_json(p.fill_color_a);
			j["fill_b"]		   = vec4_to_json(p.fill_color_b);
			j["outline_color"] = vec4_to_json(p.outline_color);
			j["rounding"]	   = p.rounding;
			j["outline"]	   = p.outline_thickness;
			j["aa"]			   = p.aa_thickness;
			j["rounding_segs"] = p.rounding_segs;
			j["gradient"]	   = static_cast<u8>(p.gradient);
			j["filled"]		   = p.filled;
			return j;
		}

		void rect_paint_from_json(const nlohmann::json& j, vg_rect_paint_t& p)
		{
			p.fill_color_a		= vec4_from_json(j.value("fill_a", nlohmann::json::array()), vec4f_t{1, 1, 1, 1});
			p.fill_color_b		= vec4_from_json(j.value("fill_b", nlohmann::json::array()), vec4f_t{1, 1, 1, 1});
			p.outline_color		= vec4_from_json(j.value("outline_color", nlohmann::json::array()), vec4f_t{0, 0, 0, 1});
			p.rounding			= j.value("rounding", 0.0f);
			p.outline_thickness = j.value("outline", 0.0f);
			p.aa_thickness		= j.value("aa", 0.0f);
			p.rounding_segs		= j.value("rounding_segs", static_cast<u16>(0));
			p.gradient			= static_cast<vg_gradient_e>(j.value("gradient", static_cast<u8>(vg_gradient_e::none)));
			p.filled			= j.value("filled", true);
		}

		nlohmann::json text_style_to_json(const vg_text_style_t& s)
		{
			nlohmann::json j;
			j["font"]		= s.font;
			j["color"]		= vec4_to_json(s.color);
			j["point_size"] = s.point_size;
			j["spacing"]	= s.spacing;
			j["raster"]		= static_cast<u8>(s.raster_mode);
			j["flip"]		= s.flip_uv;
			return j;
		}

		void text_style_from_json(const nlohmann::json& j, vg_text_style_t& s)
		{
			s.font		  = j.value<resource_handle_t>("font", NULL_RESOURCE_HANDLE);
			s.color		  = vec4_from_json(j.value("color", nlohmann::json::array()), vec4f_t{1, 1, 1, 1});
			s.point_size  = j.value("point_size", 13.0f);
			s.spacing	  = j.value("spacing", static_cast<u8>(0));
			s.raster_mode = static_cast<glyph_raster_mode_e>(j.value("raster", static_cast<u8>(glyph_raster_mode_e::lcd)));
			s.flip_uv	  = j.value("flip", false);
		}

		nlohmann::json paint_def_to_json(const paint_def_t& d, const char* text_data, u32 text_len)
		{
			nlohmann::json j;
			j["kind"]		 = static_cast<u8>(d.kind);
			j["state_flags"] = d.state_flags;
			if (d.state_flags & psf_has_hover)
				j["hover_color"] = vec4_to_json(d.hover_color);
			if (d.state_flags & psf_has_press)
				j["press_color"] = vec4_to_json(d.press_color);
			if (d.state_flags & psf_has_focus)
				j["focus_color"] = vec4_to_json(d.focus_color);

			if (d.kind == paint_kind_e::rect)
				j["rect"] = rect_paint_to_json(d.rect);
			else if (d.kind == paint_kind_e::text)
			{
				j["text"]	   = text_style_to_json(d.text);
				j["text_data"] = std::string(text_data ? text_data : "", text_len);
			}
			return j;
		}

		nlohmann::json widget_to_json(const ui_context& ui, widget_id_t id)
		{
			const layout_tree_t& tree = ui.get_tree();

			nlohmann::json j;
			j["layout"]		= layout_in_to_json(tree.in_const(id));
			j["draw_order"] = tree.draw_order_const(id);

			const paint_def_t& pd = const_cast<ui_context&>(ui).get_paint().def_const(id);
			j["paint"]			  = paint_def_to_json(pd, ui.widget_text(id), ui.widget_text_len(id));

			nlohmann::json children = nlohmann::json::array();
			widget_id_t	   c		= tree.node(id).first_child;
			while (c != NULL_WIDGET)
			{
				children.push_back(widget_to_json(ui, c));
				c = tree.node(c).next_sibling;
			}
			j["children"] = std::move(children);
			return j;
		}

		void widget_from_json(ui_context& ui, widget_id_t id, const nlohmann::json& j)
		{
			layout_in_from_json(j.at("layout"), ui.get_tree().in(id));
			ui.get_tree().draw_order(id) = j.value("draw_order", 0u);

			const auto& jp = j.at("paint");
			paint_def_t pd = {};
			pd.kind		   = static_cast<paint_kind_e>(jp.value("kind", static_cast<u8>(paint_kind_e::none)));
			pd.state_flags = jp.value("state_flags", static_cast<u8>(0));
			if (pd.state_flags & psf_has_hover)
				pd.hover_color = vec4_from_json(jp.value("hover_color", nlohmann::json::array()), vec4f_t{0, 0, 0, 0});
			if (pd.state_flags & psf_has_press)
				pd.press_color = vec4_from_json(jp.value("press_color", nlohmann::json::array()), vec4f_t{0, 0, 0, 0});
			if (pd.state_flags & psf_has_focus)
				pd.focus_color = vec4_from_json(jp.value("focus_color", nlohmann::json::array()), vec4f_t{0, 0, 0, 0});

			if (pd.kind == paint_kind_e::rect && jp.contains("rect"))
				rect_paint_from_json(jp.at("rect"), pd.rect);
			else if (pd.kind == paint_kind_e::text && jp.contains("text"))
			{
				text_style_from_json(jp.at("text"), pd.text);
				const std::string td = jp.value("text_data", std::string());
				ui.set_widget_text(id, td.c_str());
				pd.text_data = ui.widget_text(id);
				pd.text_len	 = ui.widget_text_len(id);
			}

			ui.get_paint().def(id) = pd;
			ui.get_paint().mark_text_layout_dirty(id);

			if (j.contains("children"))
			{
				for (const auto& cj : j.at("children"))
				{
					const widget_id_t cid = ui.allocate_widget();
					ui.get_tree().attach(id, cid);
					widget_from_json(ui, cid, cj);
				}
			}
		}

		void destroy_children(ui_context& ui, widget_id_t id)
		{
			layout_tree_t& tree = ui.get_tree();
			widget_id_t	   c	= tree.node(id).first_child;
			while (c != NULL_WIDGET)
			{
				const widget_id_t next = tree.node(c).next_sibling;
				ui.deallocate_widget(c);
				c = next;
			}
		}
	}

	nlohmann::json widget_hierarchy_to_json(const ui_context& ui)
	{
		const widget_id_t root = ui.get_tree().get_root();
		return widget_to_json(ui, root);
	}

	bool widget_hierarchy_from_json(ui_context& ui, const nlohmann::json& j)
	{
		try
		{
			destroy_children(ui, ui.get_tree().get_root());
			widget_from_json(ui, ui.get_tree().get_root(), j);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
}
