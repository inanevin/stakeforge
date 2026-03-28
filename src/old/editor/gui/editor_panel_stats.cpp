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

#include "editor_panel_stats.hpp"
#include "editor/editor_theme.hpp"
#include "editor/editor_settings.hpp"
#include "editor/editor.hpp"
#include "gui/vekt.hpp"
#include "common/system_info.hpp"
#include "app/app.hpp"
#include "memory/memory_tracer.hpp"

namespace SFG
{
	void editor_panel_stats::init(vekt::builder* builder)
	{
		_builder = builder;
		_gui_builder.init(builder);
	}

	void editor_panel_stats::uninit()
	{
		_gui_builder.uninit();
	}

	void editor_panel_stats::draw(const vector2ui16& window_size)
	{
		if (_stats_area == NULL_WIDGET_ID)
			return;

		static f32 stat_fetch_time = 0.0f;
		static f32 mem_fetch_time	 = 0.0f;

		static f32  stat_main_thread	 = static_cast<f32>(g_frame_info.main_thread_time_milli.load());
		static f32  stat_render_thread = static_cast<f32>(g_frame_info.render_thread_time_milli.load());
		static u32 stat_fps			 = g_frame_info.fps.load();
		static u32 stat_dc			 = static_cast<u32>(g_frame_info.draw_calls_ui.load(std::memory_order_acquire));
		static u32 stat_ram			 = 0;
		static u32 stat_vram			 = 0;
		static u32 stat_vram_txt		 = 0;
		static u32 stat_vram_res		 = 0;

		if (stat_fetch_time > 1500.0f)
		{
			stat_main_thread   = static_cast<f32>(g_frame_info.main_thread_time_milli.load());
			stat_render_thread = static_cast<f32>(g_frame_info.render_thread_time_milli.load());
			stat_fps		   = g_frame_info.fps.load();
			stat_dc			   = static_cast<u32>(g_frame_info.draw_calls_ui.load(std::memory_order_acquire));
			stat_fetch_time	   = 0.0f;

			const vector2ui16& game_size = editor::get().get_app().get_game_resolution();

			_builder->widget_append_text_start(_wv_game_res);
			_builder->widget_append_text(_wv_game_res, game_size.x, 0);
			_builder->widget_append_text(_wv_game_res, "x");
			_builder->widget_append_text(_wv_game_res, game_size.y, 0);

			_builder->widget_append_text_start(_wv_window_res);
			_builder->widget_append_text(_wv_window_res, window_size.x, 0);
			_builder->widget_append_text(_wv_window_res, "x");
			_builder->widget_append_text(_wv_window_res, window_size.y, 0);

			_builder->widget_append_text_start(_wv_fps);
			_builder->widget_append_text(_wv_fps, stat_fps);

			_builder->widget_append_text_start(_wv_main);
			_builder->widget_append_text(_wv_main, stat_main_thread);
			_builder->widget_append_text(_wv_main, " ms");

			_builder->widget_append_text_start(_wv_render);
			_builder->widget_append_text(_wv_render, stat_render_thread);
			_builder->widget_append_text(_wv_render, " ms");

			_builder->widget_append_text_start(_wv_loaded_project);
			_builder->widget_append_text(_wv_loaded_project, editor_settings::get().working_dir.c_str());

			_builder->widget_append_text_start(_wv_loaded_level);
			_builder->widget_append_text(_wv_loaded_level, editor::get().get_loaded_level().c_str());

			_builder->widget_append_text_start(_wv_draw_calls);
			_builder->widget_append_text(_wv_draw_calls, stat_dc);
		}

		if (mem_fetch_time > 6000)
		{
#ifdef SFG_ENABLE_MEMORY_TRACER
			memory_tracer& tracer = memory_tracer::get();
			LOCK_GUARD(tracer.get_category_mtx());

			for (const memory_category& cat : tracer.get_categories())
			{
				if (TO_SID(cat.name) == TO_SID("General"))
					stat_ram = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
				else if (TO_SID(cat.name) == TO_SID("Gfx"))
					stat_vram = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
				else if (TO_SID(cat.name) == TO_SID("GfxTxt"))
					stat_vram_txt = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
				else if (TO_SID(cat.name) == TO_SID("GfxRes"))
					stat_vram_res = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
			}
#endif
			mem_fetch_time = 0.0f;

			_builder->widget_append_text_start(_wv_ram);
			_builder->widget_append_text(_wv_ram, stat_ram);
			_builder->widget_append_text(_wv_ram, " mb");

			_builder->widget_append_text_start(_wv_vram);
			_builder->widget_append_text(_wv_vram, stat_vram);
			_builder->widget_append_text(_wv_vram, " mb");

			_builder->widget_append_text_start(_wv_vram_txt);
			_builder->widget_append_text(_wv_vram_txt, stat_vram_txt);
			_builder->widget_append_text(_wv_vram_txt, " mb");

			_builder->widget_append_text_start(_wv_vram_res);
			_builder->widget_append_text(_wv_vram_res, stat_vram_res);
			_builder->widget_append_text(_wv_vram_res, " mb");
		}

		const f32 ms = static_cast<f32>(g_frame_info.main_thread_time_milli.load());
		mem_fetch_time += ms;
		stat_fetch_time += ms;
	}

	void editor_panel_stats::set_visible(bool is_visible)
	{
		if (!is_visible && _stats_area != NULL_WIDGET_ID)
		{
			_builder->deallocate(_stats_area);
			_builder->build_hierarchy();
			_stats_area = NULL_WIDGET_ID;
			return;
		}

		SFG_ASSERT(_stats_area == NULL_WIDGET_ID);
		_gui_builder.set_draw_order(5);

		_stats_area = _gui_builder.begin_area(false, false);

		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_stats_area);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_end | vekt::pos_flags::pf_x_anchor_end | vekt::pos_flags::pf_child_pos_column;
			pp.pos				= {1.0f, 1.0f};

			vekt::size_props& sz = _builder->widget_get_size_props(_stats_area);
			sz.flags			 = vekt::size_flags::sf_x_relative | vekt::size_flags::sf_y_total_children;
			sz.spacing			 = editor_theme::get().item_spacing;
			sz.child_margins	 = {editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin};
			sz.size.x			 = 0.2f;

			vekt::widget_gfx& gfx = _builder->widget_get_gfx(_stats_area);
			gfx.color.w			  = 0.9f;
			gfx.draw_order		  = 2;

			_wv_game_res	   = _gui_builder.add_property_row_label("game_res:", "fetching...", 12).second;
			_wv_window_res	   = _gui_builder.add_property_row_label("window_res:", "fetching...", 12).second;
			_wv_fps			   = _gui_builder.add_property_row_label("fps:", "fetching...", 12).second;
			_wv_main		   = _gui_builder.add_property_row_label("main:", "fetching...", 12).second;
			_wv_render		   = _gui_builder.add_property_row_label("render:", "fetching...", 12).second;
			_wv_ram			   = _gui_builder.add_property_row_label("ram:", "fetching...", 12).second;
			_wv_vram		   = _gui_builder.add_property_row_label("vram:", "fetching...", 12).second;
			_wv_vram_txt	   = _gui_builder.add_property_row_label("vram_texture:", "fetching...", 12).second;
			_wv_vram_res	   = _gui_builder.add_property_row_label("vram_buffer:", "fetching...", 12).second;
			_wv_draw_calls	   = _gui_builder.add_property_row_label("draw_calls:", "fetching...", 12).second;
			_wv_loaded_project = _gui_builder.add_property_row_label("project:", editor_settings::get().working_dir.c_str(), 1024).second;
			_wv_loaded_level   = _gui_builder.add_property_row_label("world:", editor::get().get_loaded_level().c_str(), 1024).second;
			_gui_builder.set_draw_order(0);
		}
		_gui_builder.end_area();
		_builder->build_hierarchy();
	}

}
