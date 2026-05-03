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
#include <sfg/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg::ui
{
	namespace
	{
		nlohmann::json layout_in_to_json(const layout_in_t& in)
		{
			nlohmann::json j;
			j["size"]		 = in.size_value;
			j["pos"]		 = in.pos_value;
			j["margins"]	 = in.child_margins;
			j["spacing"]	 = in.child_spacing;
			j["scroll"]		 = in.scroll_offset;
			j["size_mode_x"] = static_cast<u8>(in.size_mode_x);
			j["size_mode_y"] = static_cast<u8>(in.size_mode_y);
			j["pos_mode_x"]	 = static_cast<u8>(in.pos_mode_x);
			j["pos_mode_y"]	 = static_cast<u8>(in.pos_mode_y);
			j["anchor_x"]	 = static_cast<u8>(in.anchor_x);
			j["anchor_y"]	 = static_cast<u8>(in.anchor_y);
			j["flow"]		 = static_cast<u8>(in.flow);
			j["flags"]		 = in.flags;
			return j;
		}

		void layout_in_from_json(const nlohmann::json& j, layout_in_t& in)
		{
			in.size_value	 = j.value("size", vec2f_t{0, 0});
			in.pos_value	 = j.value("pos", vec2f_t{0, 0});
			in.child_margins = j.value("margins", vec4f_t{0, 0, 0, 0});
			in.child_spacing = j.value("spacing", 0.0f);
			in.scroll_offset = j.value("scroll", 0.0f);
			in.size_mode_x	 = static_cast<axis_mode_e>(j.value("size_mode_x", static_cast<u8>(axis_mode_e::fixed)));
			in.size_mode_y	 = static_cast<axis_mode_e>(j.value("size_mode_y", static_cast<u8>(axis_mode_e::fixed)));
			in.pos_mode_x	 = static_cast<pos_mode_e>(j.value("pos_mode_x", static_cast<u8>(pos_mode_e::flow)));
			in.pos_mode_y	 = static_cast<pos_mode_e>(j.value("pos_mode_y", static_cast<u8>(pos_mode_e::flow)));
			in.anchor_x		 = static_cast<anchor_e>(j.value("anchor_x", static_cast<u8>(anchor_e::start)));
			in.anchor_y		 = static_cast<anchor_e>(j.value("anchor_y", static_cast<u8>(anchor_e::start)));
			in.flow			 = static_cast<flow_e>(j.value("flow", static_cast<u8>(flow_e::none)));
			in.flags		 = j.value("flags", static_cast<u8>(wf_visible));
		}

		nlohmann::json rect_paint_to_json(const vg_rect_paint_t& p)
		{
			nlohmann::json j;
			j["fill_a"]		   = p.fill_color_a;
			j["fill_b"]		   = p.fill_color_b;
			j["outline_color"] = p.outline_color;
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
			p.fill_color_a		= j.value("fill_a", vec4f_t{1, 1, 1, 1});
			p.fill_color_b		= j.value("fill_b", vec4f_t{1, 1, 1, 1});
			p.outline_color		= j.value("outline_color", vec4f_t{0, 0, 0, 1});
			p.rounding			= j.value("rounding", 0.0f);
			p.outline_thickness = j.value("outline", 0.0f);
			p.aa_thickness		= j.value("aa", 0.0f);
			p.rounding_segs		= j.value("rounding_segs", static_cast<u16>(0));
			p.gradient			= static_cast<vg_gradient_e>(j.value("gradient", static_cast<u8>(vg_gradient_e::none)));
			p.filled			= j.value("filled", true);
		}

		nlohmann::json text_paint_to_json(const vg_text_paint_t& p)
		{
			nlohmann::json j;
			j["color"]	 = p.color;
			j["scale"]	 = p.scale;
			j["spacing"] = p.spacing;
			j["flip"]	 = p.flip_uv;
			return j;
		}

		void text_paint_from_json(const nlohmann::json& j, vg_text_paint_t& p)
		{
			p.color	  = j.value("color", vec4f_t{1, 1, 1, 1});
			p.scale	  = j.value("scale", 1.0f);
			p.spacing = j.value("spacing", static_cast<u8>(0));
			p.flip_uv = j.value("flip", false);
			p.font	  = nullptr;
		}

		nlohmann::json paint_def_to_json(const paint_def_t& d, const char* text_data, u32 text_len)
		{
			nlohmann::json j;
			j["kind"]		 = static_cast<u8>(d.kind);
			j["state_flags"] = d.state_flags;
			if (d.state_flags & psf_has_hover)
				j["hover_color"] = d.hover_color;
			if (d.state_flags & psf_has_press)
				j["press_color"] = d.press_color;
			if (d.state_flags & psf_has_focus)
				j["focus_color"] = d.focus_color;

			if (d.kind == paint_kind_e::rect)
				j["rect"] = rect_paint_to_json(d.rect);
			else if (d.kind == paint_kind_e::text)
			{
				j["text"]	   = text_paint_to_json(d.text);
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
			while (c != INVALID_WIDGET)
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
				pd.hover_color = jp.value("hover_color", vec4f_t{0, 0, 0, 0});
			if (pd.state_flags & psf_has_press)
				pd.press_color = jp.value("press_color", vec4f_t{0, 0, 0, 0});
			if (pd.state_flags & psf_has_focus)
				pd.focus_color = jp.value("focus_color", vec4f_t{0, 0, 0, 0});

			if (pd.kind == paint_kind_e::rect && jp.contains("rect"))
				rect_paint_from_json(jp.at("rect"), pd.rect);
			else if (pd.kind == paint_kind_e::text && jp.contains("text"))
			{
				text_paint_from_json(jp.at("text"), pd.text);
				const std::string td = jp.value("text_data", std::string());
				ui.set_widget_text(id, td.c_str(), static_cast<u32>(td.size()));
				pd.text_data = ui.widget_text(id);
				pd.text_len	 = ui.widget_text_len(id);
			}

			ui.get_paint().def(id) = pd;

			if (j.contains("children"))
			{
				for (const auto& cj : j.at("children"))
				{
					const widget_id_t cid = ui.get_tree().allocate();
					ui.get_tree().attach(id, cid);
					widget_from_json(ui, cid, cj);
				}
			}
		}

		void destroy_children(layout_tree_t& tree, widget_id_t id)
		{
			widget_id_t c = tree.node(id).first_child;
			while (c != INVALID_WIDGET)
			{
				const widget_id_t next = tree.node(c).next_sibling;
				tree.deallocate(c);
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
			destroy_children(ui.get_tree(), ui.get_tree().get_root());
			widget_from_json(ui, ui.get_tree().get_root(), j);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
}
