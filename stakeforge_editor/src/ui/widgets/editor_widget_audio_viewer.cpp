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

#include "ui/widgets/editor_widget_audio_viewer.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_commands_audio.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"

#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/audio.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_widget_audio_viewer_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "audio_viewer");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		editor_misc_widgets_t::make_section_label(ui, _root, "Audio");

		_transport = ui.allocate_widget();
		ui.set_widget_debug_name(_transport, "audio_transport");
		tree.attach(_root, _transport);

		ui::layout_in_t& transport_in = tree.in(_transport);
		transport_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		transport_in.size_mode_y	  = ui::axis_mode_e::fixed;
		transport_in.size_value		  = {1.0f, theme.item_area_height};
		transport_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		transport_in.child_spacing	  = theme.item_spacing;
		transport_in.flow			  = ui::flow_e::row;

		_play_button.init(ui,
						  _transport,
						  {
							  .frame_color		   = theme.color_frame,
							  .toggled_frame_color = theme.color_accent1_dim,
							  .hover_color		   = theme.color_panel_light1,
							  .toggled_hover_color = theme.color_accent1_dim,
							  .press_color		   = theme.color_frame_light,
							  .icon_color		   = theme.color_accent1,
							  .disabled_color	   = theme.color_text_disabled,
							  .icon				   = ICON_PLAY,
							  .toggled_icon		   = ICON_PAUSE,
							  .tooltip			   = "Play or pause",
							  .on_clicked		   = on_play_clicked,
							  .user_data		   = this,
							  .size				   = theme.item_height,
							  .icon_size		   = theme.icon_default_px_size,
							  .rounding			   = theme.item_rounding,
							  .toggle_enabled	   = true,
						  });

		_reset_button.init(ui,
						   _transport,
						   {
							   .frame_color	   = theme.color_frame,
							   .hover_color	   = theme.color_panel_light1,
							   .press_color	   = theme.color_frame_light,
							   .icon_color	   = theme.color_text0,
							   .disabled_color = theme.color_text_disabled,
							   .icon		   = ICON_RESET,
							   .tooltip		   = "Return to start",
							   .on_clicked	   = on_reset_clicked,
							   .user_data	   = this,
							   .size		   = theme.item_height,
							   .icon_size	   = theme.icon_default_px_size,
							   .rounding	   = theme.item_rounding,
						   });

		_scrub_field = &_scrub_ratio;
		_scrub_slider.init(ui,
						   _transport,
						   {
							   .field		  = {.fields = {.data = &_scrub_field, .size = 1}},
							   .min_value	  = 0.0f,
							   .max_value	  = 1.0f,
							   .width		  = theme.item_width * 2.0f,
							   .decimal_count = 2,
							   .fixed_width	  = false,
							   .display_label = false,
						   });

		void* config_objects[] = {
			&_config,
		};
		editor_widget_callbacks_t callbacks = {};
		callbacks.edit_begin				= on_config_edit_begin;
		callbacks.edit_submitted			= on_config_edit_submitted;
		callbacks.user_data					= this;
		_reflection.init(ui,
						 _root,
						 {
							 .callbacks = callbacks,
							 .objects	= {.data = config_objects, .size = std::size(config_objects)},
							 .type_id	= type_id_t<audio_cook_config_t>::value,
						 });

		ui.set_pre_layout_tick(_root, on_tick, this);
		_play_button.set_disabled(true);
		_reset_button.set_disabled(true);
	}

	void editor_widget_audio_viewer_t::uninit()
	{
		clear_audio();
		_ui->clear_pre_layout_tick(_root);
		_reflection.uninit();
		_scrub_slider.uninit();
		_reset_button.uninit();
		_play_button.uninit();
		_ui->deallocate_widget(_root);

		_ui			 = nullptr;
		_root		 = NULL_WIDGET;
		_transport	 = NULL_WIDGET;
		_scrub_field = nullptr;
	}

	void editor_widget_audio_viewer_t::set_audio(sid_t audio_id)
	{
		clear_audio();

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(audio_id);

		if (asset == nullptr || asset->asset_type != editor_asset_type_e::audio)
			return;

		_audio_id						  = audio_id;
		const nlohmann::json cook_options = editor_asset_io_t::get_cook_options_json(*asset);
		reflection_registry_t::get().type_from_json(type_id_t<audio_cook_config_t>::value, &_config, nullptr, cook_options);

		void* config_objects[] = {
			&_config,
		};
		editor_widget_callbacks_t callbacks = {};
		callbacks.edit_begin				= on_config_edit_begin;
		callbacks.edit_submitted			= on_config_edit_submitted;
		callbacks.user_data					= this;
		_reflection.set_reflection({
			.callbacks = callbacks,
			.objects   = {.data = config_objects, .size = std::size(config_objects)},
			.type_id   = type_id_t<audio_cook_config_t>::value,
		});

		_resource_loaded			   = resource_manager_t::get().load_resource(audio_id, resource_type_e::audio) != resource_state_e::failed;
		const audio_runtime_t* runtime = resource_manager_t::get().find_runtime<audio_runtime_t>(audio_id);

		if (runtime != nullptr && runtime->header.sample_rate != 0)
			_duration_seconds = static_cast<f32>(runtime->header.frame_count) / static_cast<f32>(runtime->header.sample_rate);

		_play_button.set_disabled(runtime == nullptr);
		_reset_button.set_disabled(runtime == nullptr);
	}

	void editor_widget_audio_viewer_t::clear_audio()
	{
		stop_preview();

		if (_resource_loaded)
			resource_manager_t::get().unload_resource(_audio_id, false);

		_config				   = {};
		_previous_config	   = {};
		_audio_id			   = NULL_SID;
		_duration_seconds	   = 0.0f;
		_scrub_ratio		   = 0.0f;
		_displayed_scrub_ratio = 0.0f;
		_resource_loaded	   = false;

		if (_ui != nullptr)
		{
			_scrub_slider.set_value(0.0f);
			_play_button.set_disabled(true);
			_reset_button.set_disabled(true);
		}
	}

	void editor_widget_audio_viewer_t::start_preview()
	{
		const audio_runtime_t* runtime = resource_manager_t::get().find_runtime<audio_runtime_t>(_audio_id);

		if (runtime == nullptr)
			return;

		if (!audio_engine_t::get().is_voice_valid(_voice))
		{
			const audio_voice_create_desc_t desc{
				.settings =
					{
						.volume		 = 1.0f,
						.pitch		 = 1.0f,
						.attenuation = audio_attenuation_e::none,
						.bus		 = audio_bus_e::ui,
						.spatialized = false,
						.looping	 = false,
					},
				.resource = _audio_id,
				.preview  = true,
			};
			_voice = audio_engine_t::get().create_voice(*runtime, desc);
		}

		if (audio_engine_t::get().is_voice_valid(_voice))
			audio_engine_t::get().start_voice(_voice);
	}

	void editor_widget_audio_viewer_t::stop_preview()
	{
		if (audio_engine_t::get().is_voice_valid(_voice))
			audio_engine_t::get().destroy_voice(_voice);

		_voice = {};

		if (_ui != nullptr)
			_play_button.set_toggled(false);
	}

	void editor_widget_audio_viewer_t::submit_config_edit()
	{
		editor_command_audio_edit_t::edit(_audio_id, _previous_config, _config);
	}

	void editor_widget_audio_viewer_t::on_play_clicked(bool toggled, void* user_data)
	{
		editor_widget_audio_viewer_t& viewer = *static_cast<editor_widget_audio_viewer_t*>(user_data);

		if (toggled)
			viewer.start_preview();
		else if (audio_engine_t::get().is_voice_valid(viewer._voice))
			audio_engine_t::get().pause_voice(viewer._voice);
	}

	void editor_widget_audio_viewer_t::on_reset_clicked(bool toggled, void* user_data)
	{
		editor_widget_audio_viewer_t& viewer = *static_cast<editor_widget_audio_viewer_t*>(user_data);

		if (audio_engine_t::get().is_voice_valid(viewer._voice))
			audio_engine_t::get().seek_voice(viewer._voice, 0.0f);

		viewer._scrub_ratio			  = 0.0f;
		viewer._displayed_scrub_ratio = 0.0f;
		viewer._scrub_slider.set_value(0.0f);
	}

	void editor_widget_audio_viewer_t::on_config_edit_begin(void* user_data)
	{
		editor_widget_audio_viewer_t& viewer = *static_cast<editor_widget_audio_viewer_t*>(user_data);
		viewer._previous_config				 = viewer._config;
	}

	void editor_widget_audio_viewer_t::on_config_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_audio_viewer_t*>(user_data)->submit_config_edit();
	}

	void editor_widget_audio_viewer_t::on_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_widget_audio_viewer_t& viewer = *static_cast<editor_widget_audio_viewer_t*>(user_data);

		if (!audio_engine_t::get().is_voice_valid(viewer._voice))
		{
			if (!viewer._voice.is_null())
			{
				viewer._voice = {};
				viewer._play_button.set_toggled(false);
			}

			return;
		}

		if (math::abs(viewer._scrub_ratio - viewer._displayed_scrub_ratio) > 0.001f)
			audio_engine_t::get().seek_voice(viewer._voice, viewer._scrub_ratio * viewer._duration_seconds);

		const f32 cursor_seconds	  = audio_engine_t::get().get_voice_cursor_seconds(viewer._voice);
		viewer._scrub_ratio			  = viewer._duration_seconds > 0.0f ? cursor_seconds / viewer._duration_seconds : 0.0f;
		viewer._displayed_scrub_ratio = viewer._scrub_ratio;
		viewer._scrub_slider.set_value(viewer._scrub_ratio);

		if (audio_engine_t::get().is_voice_at_end(viewer._voice))
			viewer._play_button.set_toggled(false);
	}
}
