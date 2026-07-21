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
#include "editor_surface_controller.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "editor_renderer.hpp"
#include "editor_settings.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/panels/editor_panel_factory.hpp"
#include "ui/panels/editor_panel_world.hpp"
#include "ui/panels/editor_primary_base.hpp"
#include "ui/panels/editor_secondary_base.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_splash_screen.hpp"
#include "ui/widgets/editor_widget_project_creator.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "ui/widgets/editor_widget_window_frame.hpp"
#include "world/editor_world.hpp"
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/common/hashing.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define EDITOR_RAW_WHEEL_DELTA 120.0f

	namespace
	{
		editor_panel_t* find_panel_in_surface(editor_surface_t& surface, editor_panel_type_e type, sid_t sub_item_id)
		{
			if (surface.type == editor_surface_type_e::primary)
				return surface.primary->get_dock_widget().find_panel(type, sub_item_id);

			if (surface.type == editor_surface_type_e::secondary)
				return surface.secondary->get_dock_widget().find_panel(type, sub_item_id);
			return nullptr;
		}

		bool select_panel_in_surface(editor_surface_t& surface, editor_panel_t* panel)
		{
			if (surface.type == editor_surface_type_e::primary)
				return surface.primary->get_dock_widget().select_panel(panel);

			if (surface.type == editor_surface_type_e::secondary)
				return surface.secondary->get_dock_widget().select_panel(panel);
			return false;
		}

		bool add_panel_to_existing_type_leaf(editor_surface_t& surface, editor_panel_t* panel)
		{
			if (surface.type == editor_surface_type_e::primary)
				return surface.primary->get_dock_widget().dock_node_add_panel_to_existing_type_leaf(panel);

			if (surface.type == editor_surface_type_e::secondary)
				return surface.secondary->get_dock_widget().dock_node_add_panel_to_existing_type_leaf(panel);
			return false;
		}

		void on_project_ready(void* user_data)
		{
			editor_app_t& app = *static_cast<editor_app_t*>(user_data);

			editor_settings_t::get().last_project_path = editor_project_t::get()._runtime.path;
			editor_settings_t::get().save();
			engine_runtime_t::get().get_resource_file_system().set_mode_directory(editor_project_t::get()._runtime.path.c_str(), editor_directories_t::get_editor_resource_cache().c_str());
			app.request_switch_mode(editor_app_mode_e::splash);
		}
	}

	void editor_surface_controller_t::init(editor_renderer_t& renderer, editor_payload_controller_t& payload_controller)
	{
		_renderer			= &renderer;
		_payload_controller = &payload_controller;

		_debug_mode				= false;
		_close					= false;
		_cursor_capture_surface = {};
		_cursor_capture			= editor_cursor_capture_e::none;
	}

	void editor_surface_controller_t::uninit()
	{
		SFG_ASSERT(_surfaces.empty());

		_surfaces.resize_zero();

		_renderer			= nullptr;
		_payload_controller = nullptr;

		_debug_mode				= false;
		_close					= false;
		_cursor_capture_surface = {};
		_cursor_capture			= editor_cursor_capture_e::none;
	}

	void editor_surface_controller_t::load_surface_dock_layout(editor_surface_t& surface, const string_t& dock_layout)
	{
		const nlohmann::json doc = nlohmann::json::parse(dock_layout, nullptr, false);

		if (doc.is_discarded() || !doc.is_object())
			return;

		if (surface.type == editor_surface_type_e::primary)
			surface.primary->get_dock_widget().from_json(doc);
		else if (surface.type == editor_surface_type_e::secondary)
			surface.secondary->get_dock_widget().from_json(doc);
	}

	void editor_surface_controller_t::load_primary_main_toolbar(editor_surface_t& surface, const string_t& main_toolbar)
	{
		const nlohmann::json doc = nlohmann::json::parse(main_toolbar, nullptr, false);

		if (doc.is_discarded() || !doc.is_object())
			return;

		surface.primary->get_main_toolbar().deserialize(doc);
	}

	void editor_surface_controller_t::set_debug_mode(bool enabled)
	{
		_debug_mode = enabled;

		for (editor_surface_t& surface : _surfaces)
			surface.ui->set_debug_draw(enabled);
	}

	void editor_surface_controller_t::set_text_subpixel_enabled()
	{
		const ui::glyph_raster_mode_e raster_mode = editor_text_rasterization_t::get_rasterization_type();

		for (editor_surface_t& surface : _surfaces)
			surface.ui->get_paint().set_text_raster_mode(raster_mode);
	}

	void editor_surface_controller_t::begin_editor_camera_cursor_capture(window_runtime_t& runtime)
	{
		SFG_ASSERT(_cursor_capture == editor_cursor_capture_e::none);

		capture_cursor(get_surface_handle_by_runtime(runtime), editor_cursor_capture_e::editor_camera);
	}

	void editor_surface_controller_t::end_editor_camera_cursor_capture(window_runtime_t& runtime)
	{
		SFG_ASSERT(_cursor_capture == editor_cursor_capture_e::editor_camera);
		SFG_ASSERT(_cursor_capture_surface == get_surface_handle_by_runtime(runtime));

		release_cursor();
	}

	void editor_surface_controller_t::set_play_cursor_locked(bool locked)
	{
		if (!locked)
		{
			if (_cursor_capture == editor_cursor_capture_e::play)
				release_cursor();
			return;
		}

		for (editor_surface_t& surface : _surfaces)
			editor_widget_world_view_t::reset_camera_input(*surface.runtime);

		SFG_ASSERT(_cursor_capture == editor_cursor_capture_e::none);

		for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
		{
			const surface_handle_t handle  = *it;
			editor_surface_t&	   surface = _surfaces.get(handle);
			editor_panel_t* const  panel   = find_panel_in_surface(surface, editor_panel_type_e::world, 0);

			if (panel == nullptr)
				continue;

			editor_panel_world_t* const world_panel = static_cast<editor_panel_world_t*>(panel);
			process::set_cursor_position(surface.runtime->window_handle, world_panel->get_world_view().get_center());
			capture_cursor(handle, editor_cursor_capture_e::play);
			return;
		}

		SFG_ASSERT(false);
	}

	void editor_surface_controller_t::capture_cursor(surface_handle_t surface_handle, editor_cursor_capture_e capture)
	{
		SFG_ASSERT(_cursor_capture == editor_cursor_capture_e::none);
		SFG_ASSERT(capture != editor_cursor_capture_e::none);

		editor_surface_t& surface = _surfaces.get(surface_handle);

		process::set_cursor_confinement(surface.runtime->window_handle, window_cursor_confinement_e::pointer);
		process::set_cursor_visible(false);
		_cursor_capture_surface = surface_handle;
		_cursor_capture			= capture;
	}

	void editor_surface_controller_t::release_cursor()
	{
		SFG_ASSERT(_cursor_capture != editor_cursor_capture_e::none);

		editor_surface_t& surface = _surfaces.get(_cursor_capture_surface);

		process::set_cursor_confinement(surface.runtime->window_handle, window_cursor_confinement_e::none);
		process::set_cursor_visible(true);
		_cursor_capture_surface = {};
		_cursor_capture			= editor_cursor_capture_e::none;
	}

	bool editor_surface_controller_t::is_any_modal_active() const
	{
		for (const editor_surface_t& surface : _surfaces)
		{
			if (surface.modal_controller->is_visible())
				return true;
		}

		return false;
	}

	void editor_surface_controller_t::on_window_event(void*, const window_event_t& ev, void* user_data)
	{
		window_runtime_t&			 runtime		= *static_cast<window_runtime_t*>(user_data);
		editor_app_t&				 app			= editor_app_t::get();
		editor_surface_controller_t& surfaces		= editor_surface_controller_t::get();
		const surface_handle_t		 surface_handle = surfaces.get_surface_handle_by_runtime(runtime);
		editor_surface_t&			 surface		= surfaces.get_surface(surface_handle);
		ui::ui_context&				 ui				= *surface.ui;

		if (ev.type == window_event_type_e::key && ev.sub_type == window_event_sub_type_e::press && ev.button == static_cast<u16>(input_code::key_escape) && surfaces._cursor_capture == editor_cursor_capture_e::play)
		{
			surfaces.set_play_cursor_locked(false);
			return;
		}

		if (ev.type == window_event_type_e::focus && ev.sub_type == window_event_sub_type_e::release && surfaces._cursor_capture == editor_cursor_capture_e::play && surfaces._cursor_capture_surface == surface_handle)
			surfaces.set_play_cursor_locked(false);

		if (surface.type == editor_surface_type_e::splash || surface.type == editor_surface_type_e::project_creator)
		{
			switch (ev.type)
			{
			case window_event_type_e::delta:
			case window_event_type_e::mouse: {

				const vec2i16_t mp = runtime.mouse_position;
				ui.on_mouse_move({static_cast<f32>(mp.x), static_cast<f32>(mp.y)});

				if (ev.type == window_event_type_e::mouse)
				{
					if (ev.sub_type == window_event_sub_type_e::press)
						ui.on_mouse_button(ui::input_router_t::map_button(ev.button), true);
					else if (ev.sub_type == window_event_sub_type_e::release)
						ui.on_mouse_button(ui::input_router_t::map_button(ev.button), false);
				}

				break;
			}
			case window_event_type_e::wheel: {
				const f32 delta = ev.flags.is_set(static_cast<u8>(wef_high_freq)) ? static_cast<f32>(ev.value.y) / EDITOR_RAW_WHEEL_DELTA : static_cast<f32>(ev.value.y);
				ui.on_wheel(delta);
				break;
			}
			case window_event_type_e::key: {
				if (!runtime.has_flag(window_runtime_flags_e::has_focus))
					return;

				ui::key_event_t k = {};
				k.key			  = ev.button;
				k.scan_code		  = static_cast<u16>(ev.value.x);
				k.action		  = ev.sub_type == window_event_sub_type_e::press ? ui::key_action_e::press : (ev.sub_type == window_event_sub_type_e::release ? ui::key_action_e::release : ui::key_action_e::repeat);
				k.shift			  = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift));
				ui.on_key(k);
				break;
			}
			default:
				break;
			}

			return;
		}

		editor_world_controller_t& world_controller = app.get_world_controller();

		switch (ev.type)
		{
		case window_event_type_e::delta:
		case window_event_type_e::mouse: {

			const vec2i16_t mp = runtime.mouse_position;
			ui.on_mouse_move({static_cast<f32>(mp.x), static_cast<f32>(mp.y)});

			const bool modal_active = surfaces.is_any_modal_active();
			const bool popup_active = ui.get_input().is_popup_scope_active();

			if (modal_active || popup_active)
				editor_widget_world_view_t::reset_camera_input(runtime);
			else if (editor_widget_world_view_t::on_window_event(runtime, ev))
				return;

			if (ev.type == window_event_type_e::mouse)
			{
				if (ev.sub_type == window_event_sub_type_e::press)
					ui.on_mouse_button(ui::input_router_t::map_button(ev.button), true);
				else if (ev.sub_type == window_event_sub_type_e::release)
					ui.on_mouse_button(ui::input_router_t::map_button(ev.button), false);
			}

			break;
		}
		case window_event_type_e::wheel: {

			const bool modal_active = surfaces.is_any_modal_active();
			const bool popup_active = ui.get_input().is_popup_scope_active();

			if (modal_active || popup_active)
				editor_widget_world_view_t::reset_camera_input(runtime);
			else if (editor_widget_world_view_t::on_window_event(runtime, ev))
				return;

			const f32 delta = ev.flags.is_set(static_cast<u8>(wef_high_freq)) ? static_cast<f32>(ev.value.y) / EDITOR_RAW_WHEEL_DELTA : static_cast<f32>(ev.value.y);
			ui.on_wheel(delta);
			break;
		}
		case window_event_type_e::key: {
			if (!runtime.has_flag(window_runtime_flags_e::has_focus))
				return;

			const bool modal_active = surfaces.is_any_modal_active();
			const bool popup_active = ui.get_input().is_popup_scope_active();

			if (modal_active || popup_active)
				editor_widget_world_view_t::reset_camera_input(runtime);
			else if (editor_widget_world_view_t::on_window_event(runtime, ev))
				return;

			const bool					ctrl	   = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
			const editor_world_handle_t main_world = world_controller.get_main_world_handle();

			if (!modal_active && !popup_active && ctrl && ev.button == static_cast<u16>(input_code::key_s) && ev.sub_type == window_event_sub_type_e::press && !main_world.is_null() &&
				world_controller.get_editor_world(main_world)->get_edit_context().get_play_mode() == editor_play_mode_e::none)
			{
				world_controller.save_main_world();
				return;
			}

			if (!modal_active && !popup_active && app.get_command_system().on_window_event(ev))
				return;

			if (ev.button == static_cast<u16>(input_code::key_f3) && ev.sub_type == window_event_sub_type_e::press)
				app.set_debug_mode(!app.is_debug_mode_enabled());

			ui::key_event_t k = {};
			k.key			  = ev.button;
			k.scan_code		  = static_cast<u16>(ev.value.x);
			k.action		  = ev.sub_type == window_event_sub_type_e::press ? ui::key_action_e::press : (ev.sub_type == window_event_sub_type_e::release ? ui::key_action_e::release : ui::key_action_e::repeat);
			k.shift			  = process::is_key_down(static_cast<u16>(input_code::key_lshift)) || process::is_key_down(static_cast<u16>(input_code::key_rshift));
			ui.on_key(k);
			break;
		}
		case window_event_type_e::focus:
			editor_widget_world_view_t::on_window_event(runtime, ev);
			break;
		default:
			break;
		}
	}

	bool editor_surface_controller_t::on_window_client_hit_test(window_runtime_t& runtime, const vec2i16_t& pos, void*)
	{
		editor_surface_t& surface = editor_surface_controller_t::get().get_surface_by_runtime(runtime);

		if (surface.type == editor_surface_type_e::splash || surface.type == editor_surface_type_e::project_creator)
			return false;

		if (surface.type == editor_surface_type_e::primary)
			return surface.primary->is_window_drag_region(pos);

		if (surface.type == editor_surface_type_e::secondary)
			return surface.window_frame->is_window_drag_region(pos);
		return false;
	}

	void editor_surface_controller_t::on_payload_unhandled(const editor_payload_t& payload, void*)
	{
		if (payload.type != editor_payload_type_e::panel)
			return;

		editor_surface_controller_t& surfaces = editor_surface_controller_t::get();
		editor_panel_t*				 panel	  = static_cast<editor_panel_t*>(payload.user_ptr);
		vec2u16_t					 size	  = payload.size_value;

		if (size.x == 0 || size.y == 0)
			size = {640, 480};
		size.y = static_cast<u16>(size.y + editor_theme_t::get().item_height);

		const surface_handle_t surface_handle = surfaces.create_surface(payload.pos, size, editor_surface_type_e::secondary);

		if (surface_handle.is_null())
			return;

		editor_surface_t&		 surface = surfaces.get_surface(surface_handle);
		dock_widget_t&			 dock	 = surface.secondary->get_dock_widget();
		const dock_node_handle_t leaf	 = dock.create_leaf_node(dock.get_root());
		dock.set_root_node(leaf);
		dock.dock_node_add_panel(leaf, panel);
	}

	surface_handle_t editor_surface_controller_t::create_surface(const vec2i16_t& pos, const vec2u16_t& size, editor_surface_type_e type)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

		_renderer->end_render();

		if (size.x == 0 || size.y == 0)
		{
			SFG_ERR("can't create a surface with zero size.");
			return {};
		}

		const surface_handle_t handle  = _surfaces.add();
		editor_surface_t&	   surface = _surfaces.get(handle);
		surface.type				   = type;
		surface.runtime				   = make_unique<window_runtime_t>();

		const window_style_e window_style = type == editor_surface_type_e::payload ? window_style_e::alpha : window_style_e::borderless;

		if (!process::create_window("Stakeforge Editor", pos, size, window_style, 0.75f, type == editor_surface_type_e::payload, *surface.runtime))
		{
			SFG_ERR("failed creating editor surface window!");
			_surfaces.remove(handle);
			return {};
		}

		surface.runtime->set_flag(window_runtime_flags_e::high_frequency_input);

		surface.ui = make_unique<ui::ui_context>();

		surface.ui->init({
			.canvas =
				{
					.vertex_buffer_bytes = 1 << 24,
					.index_buffer_bytes	 = 1 << 24,
					.buffer_count		 = 64,
				},
			.user_ui_scale		= 1.0f,
			.dpi_scale			= surface.runtime->monitor_info.dpi_scale,
			.max_widgets		= 10000,
			.text_pool_capacity = 1024 * 1024,
		});

		surface.ui->get_paint().set_pipelines({
			.default_pipeline		 = "editor/resource_pack/shaders/editor_ui_default.hlsl"_hs,
			.text_pipeline			 = "editor/resource_pack/shaders/editor_ui_text_lcd.hlsl"_hs,
			.grayscale_text_pipeline = "editor/resource_pack/shaders/editor_ui_text_grayscale.hlsl"_hs,
			.sdf_pipeline			 = "editor/resource_pack/shaders/editor_ui_sdf.hlsl"_hs,
		});

		surface.ui->set_debug_draw(_debug_mode);

		surface.root = surface.ui->allocate_widget();
		surface.ui->set_widget_debug_name(surface.root, "surface_root");
		surface.ui->get_tree().attach(surface.ui->get_root(), surface.root);

		ui::layout_in_t& surface_root_in = surface.ui->get_tree().in(surface.root);
		surface_root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		surface_root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		surface_root_in.size_value		 = {1.0f, 1.0f};
		surface_root_in.flow			 = ui::flow_e::column;
		surface_root_in.child_spacing	 = 0.0f;
		surface_root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		editor_theme_t& theme = editor_theme_t::get();

		surface.content_root = surface.root;

		if (surface.type == editor_surface_type_e::secondary || surface.type == editor_surface_type_e::project_creator)
		{
			surface.window_frame = make_unique<editor_widget_window_frame_t>();
			surface.window_frame->init(*surface.ui, surface.root, {.runtime = surface.runtime.get(), .only_close = surface.type == editor_surface_type_e::project_creator});

			surface.owner_root = surface.ui->allocate_widget();
			surface.ui->set_widget_debug_name(surface.owner_root, "surface_owner_root");
			surface.ui->get_tree().attach(surface.root, surface.owner_root);

			ui::layout_in_t& owner_root_in = surface.ui->get_tree().in(surface.owner_root);
			owner_root_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
			owner_root_in.size_mode_y	   = ui::axis_mode_e::fill;
			owner_root_in.size_value	   = {1.0f, 1.0f};
			owner_root_in.flow			   = ui::flow_e::column;
			owner_root_in.child_margins	   = {1, 1, 1, 1};

			const ui::vg_rect_paint_t owner_paint = {
				.fill_color_a = theme.color_frame_dark,
				.fill_color_b = theme.color_frame_dark,
			};

			surface.ui->get_paint().set_rect(surface.owner_root, owner_paint);

			surface.content_root = surface.ui->allocate_widget();
			surface.ui->set_widget_debug_name(surface.content_root, "surface_content_root");
			surface.ui->get_tree().attach(surface.owner_root, surface.content_root);

			ui::layout_in_t& content_root_in = surface.ui->get_tree().in(surface.content_root);
			content_root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
			content_root_in.size_mode_y		 = ui::axis_mode_e::fill;
			content_root_in.size_value		 = {1.0f, 1.0f};
			content_root_in.flow			 = ui::flow_e::column;
			content_root_in.child_spacing	 = 0.0f;
			content_root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};
		}

		surface.tooltip_controller = make_unique<editor_tooltip_controller_t>();
		surface.tooltip_controller->init(*surface.ui);

		surface.modal_controller = make_unique<editor_modal_controller_t>();
		surface.modal_controller->init(*surface.ui);

		surface.popup_controller = make_unique<editor_popup_controller_t>();
		surface.popup_controller->init(*surface.ui);

		surface.action_menu_controller = make_unique<editor_action_menu_controller_t>();
		surface.action_menu_controller->init(*surface.ui);

		if (surface.type == editor_surface_type_e::primary)
		{
			surface.primary = make_unique<editor_primary_base_t>();
			surface.primary->init(*surface.ui, surface.content_root, *surface.runtime);
		}
		else if (surface.type == editor_surface_type_e::secondary)
		{
			surface.secondary = make_unique<editor_secondary_base_t>();
			surface.secondary->init(*surface.ui, surface.content_root, *surface.runtime);
		}
		else if (surface.type == editor_surface_type_e::splash)
		{
			surface.splash = make_unique<editor_splash_screen_t>();
			surface.splash->init(*surface.ui, surface.content_root, {.owner_size = surface.runtime->size});
		}
		else if (surface.type == editor_surface_type_e::project_creator)
		{
			surface.project_creator = make_unique<editor_widget_project_creator_t>();
			surface.project_creator->init(*surface.ui, surface.content_root, {.on_project_ready = on_project_ready, .user_data = &editor_app_t::get()});
		}

		surface.swapchain	   = _renderer->create_swapchain(surface.runtime->window_handle, surface.runtime->platform_handle, surface.runtime->monitor_info.dpi_scale, surface.runtime->size, surface.ui.get());
		surface.swapchain_size = surface.runtime->size;

		surface.runtime->event_callback			   = &editor_surface_controller_t::on_window_event;
		surface.runtime->event_callback_user_data  = surface.runtime.get();
		surface.runtime->client_hit_test_callback  = &editor_surface_controller_t::on_window_client_hit_test;
		surface.runtime->client_hit_test_user_data = this;

		return handle;
	}

	void editor_surface_controller_t::destroy_surface(surface_handle_t handle)
	{
		SFG_ASSERT(SFG_IS_MAIN_THREAD());

		_renderer->end_render();

		editor_surface_t& surface = _surfaces.get(handle);

		if (handle == _cursor_capture_surface)
		{
			if (_cursor_capture == editor_cursor_capture_e::play)
				set_play_cursor_locked(false);
			else
				editor_widget_world_view_t::reset_camera_input(*surface.runtime);
		}

		if (surface.type == editor_surface_type_e::primary)
			save_layout();

		if (surface.type == editor_surface_type_e::primary)
			surface.primary->uninit();
		else if (surface.type == editor_surface_type_e::secondary)
			surface.secondary->uninit();
		else if (surface.type == editor_surface_type_e::payload)
			_payload_controller->uninit();
		else if (surface.type == editor_surface_type_e::splash)
			surface.splash->uninit();
		else if (surface.type == editor_surface_type_e::project_creator)
			surface.project_creator->uninit();

		if (surface.window_frame)
			surface.window_frame->uninit();

		surface.ui->deallocate_widget(surface.root);

		surface.tooltip_controller->uninit();
		surface.popup_controller->uninit();
		surface.modal_controller->uninit();
		surface.action_menu_controller->uninit();
		surface.ui->uninit();
		surface.ui.reset();
		_renderer->destroy_swapchain(surface.swapchain);
		surface.swapchain = {};
		process::destroy_window(surface.runtime->window_handle);
		_surfaces.remove(handle);
	}

	void editor_surface_controller_t::destroy_all_surfaces()
	{
		frame_vector_t<surface_handle_t> destroy_handles = {};
		frame_vector_t<surface_handle_t> payload_handles = {};

		for (u16 i = 0; i < _surfaces.head(); ++i)
		{
			if (!_surfaces.is_active(i))
				continue;

			const surface_handle_t	handle	= _surfaces.get_handle(i);
			const editor_surface_t& surface = _surfaces.get(handle);

			if (surface.type == editor_surface_type_e::payload)
				payload_handles.push_back(handle);
			else
				destroy_handles.push_back(handle);
		}

		for (surface_handle_t handle : destroy_handles)
			destroy_surface(handle);

		for (surface_handle_t handle : payload_handles)
			destroy_surface(handle);
	}

	void editor_surface_controller_t::tick_surfaces(f32 dt)
	{
		for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
		{
			const surface_handle_t handle  = *it;
			editor_surface_t&	   surface = _surfaces.get(handle);

			if (surface.runtime->has_flag(window_runtime_flags_e::close_requested) || _close)
			{
				_renderer->end_render();

				if (surface.type == editor_surface_type_e::primary)
					_close = true;

				destroy_surface(handle);
				continue;
			}

			const bool minimized = surface.runtime->has_flag(window_runtime_flags_e::minimized);
			const bool hidden	 = surface.runtime->is_hidden;

			if (minimized != surface.is_minimized)
			{
				surface.is_minimized = minimized;
				_renderer->end_render();
				_renderer->set_swapchain_minimized(surface.swapchain, minimized);
			}

			if (hidden != surface.is_hidden)
			{
				surface.is_hidden = hidden;
				_renderer->end_render();
				_renderer->set_swapchain_visible(surface.swapchain, !hidden);
			}

			if (!minimized && !hidden && surface.runtime->size != surface.swapchain_size)
			{
				_renderer->end_render();
				_renderer->resize_swapchain(surface.swapchain, surface.runtime->size, surface.runtime->monitor_info.dpi_scale);
				surface.swapchain_size = surface.runtime->size;
			}

			if (!minimized && !hidden)
			{
				const vec4f_t screen	= {0.0f, 0.0f, static_cast<f32>(surface.swapchain_size.x), static_cast<f32>(surface.swapchain_size.y)};
				const f32	  dpi_scale = surface.runtime->monitor_info.dpi_scale > 0.0f ? surface.runtime->monitor_info.dpi_scale : 1.0f;
				surface.ui->tick(screen, dpi_scale, dt);
				surface.ui->publish_frame();
			}
		}
	}

	void editor_surface_controller_t::save_layout()
	{
		editor_layout_t& layout = editor_settings_t::get().layout;
		layout					= {};

		bool primary_saved = false;

		for (const editor_surface_t& surface : _surfaces)
		{
			if (surface.type == editor_surface_type_e::payload || surface.type == editor_surface_type_e::splash || surface.type == editor_surface_type_e::project_creator)
				continue;

			editor_layout_window_t window = {};
			window.pos					  = surface.runtime->pos;
			window.size					  = surface.runtime->size;
			window.is_primary			  = surface.type == editor_surface_type_e::primary;
			window.maximized			  = surface.runtime->has_flag(window_runtime_flags_e::maximized);

			if (surface.type == editor_surface_type_e::primary)
			{
				window.dock_layout			= string_t(surface.primary->get_dock_widget().to_json().dump());
				nlohmann::json main_toolbar = {};
				surface.primary->get_main_toolbar().serialize(main_toolbar);
				window.main_toolbar = string_t(main_toolbar.dump());
			}
			else if (surface.type == editor_surface_type_e::secondary)
				window.dock_layout = string_t(surface.secondary->get_dock_widget().to_json().dump());

			if (window.is_primary)
			{
				SFG_ASSERT(!primary_saved);
				primary_saved = true;
			}

			layout.windows.push_back(window);
		}

		editor_settings_t::get().save();
	}

	void editor_surface_controller_t::apply_default_layout()
	{
		frame_vector_t<surface_handle_t> destroy_handles = {};

		for (u16 i = 0; i < _surfaces.head(); ++i)
		{
			if (!_surfaces.is_active(i))
				continue;

			const surface_handle_t	handle	= _surfaces.get_handle(i);
			const editor_surface_t& surface = _surfaces.get(handle);

			if (surface.type == editor_surface_type_e::secondary)
				destroy_handles.push_back(handle);
		}

		for (surface_handle_t handle : destroy_handles)
			destroy_surface(handle);

		editor_layout_t::load_surface_default_layout(get_main_surface());
		save_layout();
	}

	editor_panel_t* editor_surface_controller_t::find_panel(editor_panel_type_e type, surface_handle_t surface_handle, sid_t sub_item_id)
	{
		if (!surface_handle.is_null())
		{
			editor_panel_t* panel = find_panel_in_surface(_surfaces.get(surface_handle), type, sub_item_id);

			if (panel != nullptr)
				return panel;

			for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
			{
				const surface_handle_t handle = *it;

				if (handle == surface_handle)
					continue;

				panel = find_panel_in_surface(_surfaces.get(handle), type, sub_item_id);

				if (panel != nullptr)
					return panel;
			}

			return nullptr;
		}

		surface_handle_t main_surface = {};

		for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
		{
			const surface_handle_t handle  = *it;
			editor_surface_t&	   surface = _surfaces.get(handle);

			if (surface.type != editor_surface_type_e::primary)
				continue;

			main_surface		  = handle;
			editor_panel_t* panel = find_panel_in_surface(surface, type, sub_item_id);

			if (panel != nullptr)
				return panel;
			break;
		}

		for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
		{
			const surface_handle_t handle = *it;

			if (handle == main_surface)
				continue;

			editor_panel_t* panel = find_panel_in_surface(_surfaces.get(handle), type, sub_item_id);

			if (panel != nullptr)
				return panel;
		}

		return nullptr;
	}

	editor_panel_t* editor_surface_controller_t::find_panel_on_surface(editor_panel_type_e type, surface_handle_t surface_handle, sid_t sub_item_id)
	{
		return find_panel_in_surface(_surfaces.get(surface_handle), type, sub_item_id);
	}

	editor_panel_t* editor_surface_controller_t::create_panel_instance(editor_panel_type_e type, surface_handle_t surface_handle, bool prefer_existing_type_dock, sid_t sub_item_id)
	{
		if (sub_item_id != 0)
		{
			editor_panel_t* panel = find_panel(type, surface_handle, sub_item_id);

			if (panel != nullptr)
			{
				for (editor_surface_t& surface : _surfaces)
				{
					if (!select_panel_in_surface(surface, panel))
						continue;

					process::bring_to_front(surface.runtime->window_handle);
					return panel;
				}

				SFG_ASSERT(false);
				return nullptr;
			}
		}

		editor_panel_t* panel = editor_panel_factory_t::create_panel(type);

		if (panel == nullptr)
			return nullptr;
		panel->set_sub_item_id(sub_item_id);

		if (prefer_existing_type_dock)
		{
			if (!surface_handle.is_null())
			{
				editor_surface_t& surface = _surfaces.get(surface_handle);

				if (add_panel_to_existing_type_leaf(surface, panel))
				{
					process::bring_to_front(surface.runtime->window_handle);
					return panel;
				}
			}

			for (editor_surface_t& surface : _surfaces)
			{
				if (add_panel_to_existing_type_leaf(surface, panel))
				{
					process::bring_to_front(surface.runtime->window_handle);
					return panel;
				}
			}
		}

		const editor_surface_t& main_surface = get_main_surface();
		const vec2i16_t			pos			 = {static_cast<i16>(main_surface.runtime->pos.x + 64), static_cast<i16>(main_surface.runtime->pos.y + 64)};
		const vec2u16_t			size		 = {640, 480};
		const surface_handle_t	new_surface	 = create_surface(pos, size, editor_surface_type_e::secondary);

		if (new_surface.is_null())
		{
			editor_panel_factory_t::delete_panel(panel);
			return nullptr;
		}

		editor_surface_t&		 surface = _surfaces.get(new_surface);
		dock_widget_t&			 dock	 = surface.secondary->get_dock_widget();
		const dock_node_handle_t leaf	 = dock.create_leaf_node(dock.get_root());
		dock.set_root_node(leaf);
		dock.dock_node_add_panel(leaf, panel);
		process::bring_to_front(surface.runtime->window_handle);
		return panel;
	}

	editor_panel_t* editor_surface_controller_t::show_panel(editor_panel_type_e type, surface_handle_t surface_handle, sid_t sub_item_id)
	{
		const editor_panel_type_desc_t& desc = editor_panel_factory_t::get_desc(type);

		if (desc.allows_multiple_instances)
			return create_panel_instance(type, surface_handle, true, sub_item_id);

		editor_panel_t* panel = find_panel(type, surface_handle, sub_item_id);

		if (panel != nullptr)
		{
			for (editor_surface_t& surface : _surfaces)
			{
				if (!select_panel_in_surface(surface, panel))
					continue;

				process::bring_to_front(surface.runtime->window_handle);
				return panel;
			}

			SFG_ASSERT(false);
			return nullptr;
		}

		return create_panel_instance(type, surface_handle, true, sub_item_id);
	}

	void editor_surface_controller_t::refresh_panel_title(editor_panel_t* panel)
	{
		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.type == editor_surface_type_e::primary && surface.primary->get_dock_widget().refresh_panel_title(panel))
				return;

			if (surface.type == editor_surface_type_e::secondary && surface.secondary->get_dock_widget().refresh_panel_title(panel))
				return;
		}

		SFG_ASSERT(false);
	}

	editor_surface_t& editor_surface_controller_t::get_main_surface()
	{
		SFG_ASSERT(!_surfaces.empty());

		for (editor_surface_t& surface : _surfaces)
		{
			if (surface.type == editor_surface_type_e::primary)
				return surface;
		}

		SFG_ASSERT(false);
		return *_surfaces.begin();
	}

	editor_surface_t& editor_surface_controller_t::get_surface_by_runtime(window_runtime_t& runtime)
	{
		return _surfaces.get(get_surface_handle_by_runtime(runtime));
	}

	surface_handle_t editor_surface_controller_t::get_surface_handle_by_runtime(window_runtime_t& runtime)
	{
		for (auto it = _surfaces.begin_handle(); it != _surfaces.end_handle(); ++it)
		{
			const surface_handle_t handle  = *it;
			editor_surface_t&	   surface = _surfaces.get(handle);

			if (surface.runtime.get() == &runtime)
				return handle;
		}

		SFG_ASSERT(false);
		return _surfaces.begin_handle() != _surfaces.end_handle() ? *_surfaces.begin_handle() : surface_handle_t{};
	}
}
