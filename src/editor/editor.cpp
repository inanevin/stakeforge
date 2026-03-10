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

#include "editor.hpp"
#include "editor_settings.hpp"
#include "editor_theme.hpp"
#include "editor_layout.hpp"

// app
#include "app/app.hpp"

// gfx
#include "gfx/renderer.hpp"

// platform
#include "platform/window.hpp"
#include "platform/process.hpp"

// world
#include "world/world.hpp"
#include "resources/world_raw.hpp"

// resources
#include "resources/resource_manager.hpp"
#include "resources/model.hpp"
#include "resources/entity_template.hpp"
#include "resources/entity_template_raw.hpp"

// components

// gui
#include "gui/vekt.hpp"
#include "editor/gui/editor_gui_controller.hpp"
#include "editor/gui/editor_panel_entities.hpp"

// misc
#include "serialization/serialization.hpp"
#include "data/char_util.hpp"
#include "io/file_system.hpp"
#include "input/input_mappings.hpp"
#include "math/math.hpp"
#include "game/gameplay.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{

#define EDITOR_SETTINGS_FILE "/stakeforge/editor.stksettings"
	editor::editor(app& game) : _app(game)
	{
	}

	editor::~editor()
	{
		_playmode_backup.destroy();
	}

	editor* editor::s_instance = nullptr;
	namespace
	{

		string escape_braces(std::string_view s)
		{
			std::string out;
			out.reserve(s.size()); // will grow if it finds braces
			for (char c : s)
			{
				if (c == '{' || c == '}')
					out.push_back(c);
				out.push_back(c);
			}
			return out;
		}

		void vekt_log(vekt::log_verbosity verb, const char* err, ...)
		{
			va_list args;
			va_start(args, err);

			char buffer[4096];
			vsnprintf(buffer, sizeof(buffer), err, args);

			std::string msg(buffer);
			msg = escape_braces(msg);

			if (verb == vekt::log_verbosity::error)
				SFG_ERR(msg.c_str());
			else
				SFG_INFO(msg.c_str());

			va_end(args);
		}
	}

	void editor::init()
	{
		s_instance = this;

		// -----------------------------------------------------------------------------
		// vekt init
		// -----------------------------------------------------------------------------

		vekt::config.on_log = vekt_log;

		_builder = new vekt::builder();
		_builder->set_callback_user_data(this);
		_builder->set_on_allocate_text(on_vekt_allocate_text);
		_builder->set_on_deallocate_text(on_vekt_deallocate_text);
		constexpr size_t VTX_SZ = 1024 * 1024 * 32;
		constexpr size_t IDX_SZ = 1024 * 1024 * 24;
		_builder->init({
			.vertex_buffer_sz			 = VTX_SZ,
			.index_buffer_sz			 = IDX_SZ,
			.text_cache_vertex_buffer_sz = 1024 * 1024 * 2,
			.text_cache_index_buffer_sz	 = 1024 * 1024 * 4,
			.buffer_count				 = 120,
		});

		// font
		_font_manager = new vekt::font_manager();
		_font_manager->init();
		_font_manager->set_callback_user_data(&_renderer);
		_font_manager->set_atlas_created_callback(editor_renderer::on_atlas_created);
		_font_manager->set_atlas_updated_callback(editor_renderer::on_atlas_updated);
		_font_manager->set_atlas_destroyed_callback(editor_renderer::on_atlas_destroyed);

		_renderer.init(_app.get_main_window(), _app.get_renderer().get_texture_queue(), VTX_SZ, IDX_SZ);

		editor_theme::UI_SCALING = window::UI_SCALE;

		const f32 base_text_height = 14 * window::UI_SCALE;

		const string default_font_str = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/Quantico-Regular.ttf");
		const string title_font_str	  = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/VT323-Regular.ttf");
		const string icon_font_str	  = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/icons.ttf");
		_font_main					  = _font_manager->load_font_from_file(default_font_str.c_str(), base_text_height);
		_font_title					  = _font_manager->load_font_from_file(title_font_str.c_str(), base_text_height + base_text_height * 0.7f);
		_font_icons					  = _font_manager->load_font_from_file(icon_font_str.c_str(), base_text_height + base_text_height * 0.2f, 32, 128, vekt::font_type::sdf, 3, 128, 32);

		world_debug_rendering& wdr = editor::get().get_app().get_world().get_debug_rendering();
		wdr.set_default_font(_font_main, _renderer.get_atlas_gpu_index(_font_main->_atlas));
		wdr.set_icon_font(_font_icons, _renderer.get_atlas_gpu_index(_font_icons->_atlas));

		// -----------------------------------------------------------------------------
		// editor pipeline
		// -----------------------------------------------------------------------------

		if (!file_system::exists(editor_settings::get().working_dir.c_str()))
		{
			editor_settings::get().working_dir = process::select_folder("select project directory");
			file_system::fix_path(editor_settings::get().working_dir);
			editor_settings::get().save_last();

			if (!file_system::exists(editor_settings::get().working_dir.c_str()))
			{
				SFG_ASSERT(false);
			}
		}
		editor_theme::get().init(editor_settings::get()._editor_folder.c_str());
		editor_theme::get().apply_scaling();

		editor_layout::get().init(editor_settings::get()._editor_folder.c_str());

		editor_theme::get().font_default = _font_main;
		editor_theme::get().font_title	 = _font_title;
		editor_theme::get().font_icons	 = _font_icons;

		_camera_controller.init(_app.get_world(), _app.get_main_window());
		_app.get_world().init();

		if (!editor_settings::get().last_world_relative.empty())
		{
			const string last_world = editor_settings::get().working_dir + editor_settings::get().last_world_relative;
			if (file_system::exists(last_world.c_str()))
				load_level(editor_settings::get().last_world_relative.c_str());
			else
				_camera_controller.activate();
		}
		else
			_camera_controller.activate();

		// -----------------------------------------------------------------------------
		// gui
		// -----------------------------------------------------------------------------
		_bump_text_allocator.init(1024 * 512);
		_text_allocator.init(1024 * 1024);
		_gui_controller.init(_builder);
	}

	void editor::uninit()
	{
		editor_layout::get().save_last();
		_gui_controller.uninit();

		_bump_text_allocator.uninit();
		_text_allocator.uninit();

		editor_theme::get().font_default = nullptr;
		editor_theme::get().font_title	 = nullptr;

		_font_manager->unload_font(_font_main);
		_font_manager->unload_font(_font_title);
		_font_manager->unload_font(_font_icons);
		_font_manager->uninit();
		delete _font_manager;
		_font_manager = nullptr;

		_builder->uninit();
		delete _builder;
		_builder = nullptr;

		_renderer.uninit();

		_camera_controller.uninit();
	}

	void editor::post_world_tick(f32 delta)
	{
		_camera_controller.tick(delta);
		world& w = _app.get_world();
		if (w.get_playmode() == play_mode::full)
			_app.get_gameplay().on_world_tick(w, delta, _app.get_game_resolution());
	}

	void editor::tick()
	{
		ZoneScoped;
		window&				  wnd	= _app.get_main_window();
		const vector<string>& drops = wnd.get_dropped_files();
		for (const string& str : drops)
			on_file_dropped(str.c_str());
		wnd.clear_dropped_files();

		world&			   w  = _app.get_world();
		const vector2ui16& ws = _app.get_main_window().get_size();

		const vector2	  wvs = _gui_controller.get_world_size();
		const vector2ui16 gs  = vector2ui16(wvs.x, wvs.y);
		_app.set_game_resolution(gs);

		_bump_text_allocator.reset();

		_gui_controller.tick(w, ws);
		_builder->build_begin(vector2(ws.x, ws.y));
		_builder->build_end();

		// notify buffer swap
		_renderer.draw_end(_builder);
	}

	void editor::render(const render_params& p)
	{
		ZoneScoped;

		_renderer.prepare(p.pm, p.cmd_buffer, p.frame_index);
		_renderer.render({
			.cmd_buffer		   = p.cmd_buffer,
			.frame_index	   = p.frame_index,
			.alloc			   = p.alloc,
			.size			   = p.size,
			.global_layout	   = p.global_layout,
			.global_group	   = p.global_group,
			.world_rt_index	   = p.world_rt_index,
			.color_rt_index	   = p.color_rt_index,
			.normals_rt_index  = p.normals_rt_index,
			.orm_rt_index	   = p.orm_rt_index,
			.emissive_rt_index = p.emissive_rt_index,
			.lighting_rt_index = p.lighting_rt_index,
			.ssao_rt_index	   = p.ssao_rt_index,
			.bloom_rt_index	   = p.bloom_rt_index,
			.depth_rt_index	   = p.depth_rt_index,

		});
	}

	bool editor::on_window_event(const window_event& ev)
	{
		if (ev.type == window_event_type::display_change)
		{
			on_display_change();
			return true;
		}

		if (ev.type == window_event_type::mouse && ev.button == input_code::mouse_1 && ev.sub_type == window_event_sub_type::release)
		{
			_camera_controller.set_is_looking(false);
			return true;
		}

		const play_mode pm = _app.get_world().get_playmode();

		if (_camera_controller.get_is_looking())
		{
			_camera_controller.on_window_event(ev);
			return true;
		}

		if (ev.type == window_event_type::key && ev.button == input_code::key_escape && ev.sub_type == window_event_sub_type::press)
		{
			if (pm == play_mode::full)
			{
				_gui_controller.on_exited_playmode();
				exit_playmode();
				return true;
			}

			_app.get_main_window().confine_cursor(cursor_confinement::none);
			_app.get_main_window().set_cursor_visible(true);

			return true;
		}

		if (pm == play_mode::full)
			return false;

		if (ev.type == window_event_type::delta)
		{
			const vector2i16& mp  = _app.get_main_window().get_mouse_position();
			const vector2	  mpf = vector2(mp.x, mp.y);
			if (_gui_controller.on_mouse_move(mpf))
				return true;

			_builder->on_mouse_move(mpf);
		}
		else if (ev.type == window_event_type::wheel)
		{
			_builder->on_mouse_wheel_event({.amount = 12 * static_cast<f32>(-ev.value.y) / window::get_wheel_delta()});
		}
		else if (ev.type == window_event_type::mouse)
		{
			if (_gui_controller.on_mouse_event(ev))
				return true;
		}
		else if (ev.type == window_event_type::key)
		{
			if (ev.button == input_code::key_tab && ev.sub_type == window_event_sub_type::press)
			{
				if (window::is_key_down(input_code::key_lshift))
					_builder->prev_focus();
				else
					_builder->next_focus();
			}
			else
			{
				if (_gui_controller.on_key_event(ev))
					return true;

				const vekt::input_event_result res = _builder->on_key_event({
					.type	   = static_cast<vekt::input_event_type>(ev.sub_type),
					.key	   = ev.button,
					.scan_code = ev.value.x,
				});

				if (res == vekt::input_event_result::handled)
					return true;
			}
		}

		_camera_controller.on_window_event(ev);
		return true;
	}

	void editor::on_display_change()
	{
		editor_theme::UI_SCALING = window::UI_SCALE;
		editor_theme::get().init(editor_settings::get()._editor_folder.c_str());
		editor_theme::get().apply_scaling();

		_gui_controller.uninit();

		_font_manager->unload_font(_font_main);
		_font_manager->unload_font(_font_title);
		_font_manager->unload_font(_font_icons);

		const f32	 base_text_height = 14 * window::UI_SCALE;
		const string default_font_str = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/Quantico-Regular.ttf");
		const string title_font_str	  = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/VT323-Regular.ttf");
		const string icon_font_str	  = SFG_ROOT_DIRECTORY + string("assets/engine/fonts/icons.ttf");
		_font_main					  = _font_manager->load_font_from_file(default_font_str.c_str(), base_text_height);
		_font_title					  = _font_manager->load_font_from_file(title_font_str.c_str(), base_text_height + base_text_height * 0.7f);
		_font_icons					  = _font_manager->load_font_from_file(icon_font_str.c_str(), base_text_height + base_text_height * 0.2f, 32, 128, vekt::font_type::sdf, 3, 128, 32);

		editor_theme::get().font_default = _font_main;
		editor_theme::get().font_title	 = _font_title;
		editor_theme::get().font_icons	 = _font_icons;

		world_debug_rendering& wdr = editor::get().get_app().get_world().get_debug_rendering();
		wdr.set_default_font(_font_main, _renderer.get_atlas_gpu_index(_font_main->_atlas));
		wdr.set_icon_font(_font_icons, _renderer.get_atlas_gpu_index(_font_icons->_atlas));

		_gui_controller.init(_builder);
	}

	void editor::resize(const vector2ui16& size)
	{
		_renderer.resize(size);
	}

	void editor::on_file_dropped(const char* path)
	{
		const string& wd = editor_settings::get().working_dir;

		string file = path;
		file_system::fix_path(file);

		if (!editor_settings::get().is_in_work_directory(file))
		{
			SFG_ERR("file should be in project directory.");
			return;
		}

		const string relative = file.substr(wd.size(), file.length() - wd.size());
		const string ext	  = file_system::get_file_extension(relative);

		world&			   w  = _app.get_world();
		resource_manager&  rm = w.get_resource_manager();
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();

		if (ext.compare("stkworld") == 0)
		{
			load_level(relative.c_str());
			return;
		}

		const world_handle camera_entity = _camera_controller.get_entity();
		const vector3&	   cam_pos		 = em.get_entity_position(camera_entity);
		const quat&		   cam_rot		 = em.get_entity_rotation(camera_entity);
		const vector3	   placement_pos = cam_pos + cam_rot.get_forward() * 3.0f;

		if (ext.compare("stkent") == 0)
		{
			const string_id sid	   = TO_SID(relative);
			resource_handle handle = rm.get_resource_handle_by_hash_if_exists<entity_template>(sid);

			if (handle.is_null())
			{
				rm.load_resources({relative}, false, wd.c_str());
				handle = rm.get_resource_handle_by_hash<entity_template>(sid);
			}

			if (handle.is_null())
			{
				SFG_ERR("something went wrong loading resource: {0}", relative.c_str());
				return;
			}

			const world_handle instantiated = em.instantiate_template(handle);
			em.set_entity_position(instantiated, placement_pos);
			em.teleport_entity(instantiated);
			_gui_controller.set_selected_entity(instantiated);

			return;
		}

		if (ext.compare("stkmodel") == 0)
		{
			const string_id sid = TO_SID(relative);

			resource_handle handle = rm.get_resource_handle_by_hash_if_exists<model>(sid);

			if (handle.is_null())
			{
				rm.load_resources({relative}, false, wd.c_str());
				handle = rm.get_resource_handle_by_hash<model>(sid);
			}

			if (handle.is_null())
			{
				SFG_ERR("something went wrong loading resource: {0}", relative.c_str());
				return;
			}

			const world_handle instantiated = em.instantiate_model(handle);
			em.set_entity_position(instantiated, placement_pos);
			em.teleport_entity(instantiated);

			_gui_controller.set_selected_entity(instantiated);

			return;
		}

		SFG_ERR("dropped file with unknown extension: {0}", ext.c_str());
	}

	const vector2ui16& editor::get_game_resolution() const
	{
		return _app.get_game_resolution();
	}

	void editor::load_level_prompt()
	{
		string file = process::select_file("load level", ".stkworld");
		file_system::fix_path(file);

		const string& work_dir = editor_settings::get().working_dir;
		if (!editor_settings::get().is_in_work_directory(file))
		{
			SFG_ERR("level should be in project directory.");
			return;
		}
		const string relative = file.substr(work_dir.size(), file.length() - work_dir.size());

		load_level(relative.c_str());
	}

	void editor::load_level(const char* relative_path)
	{
		const string file = relative_path;

		_gui_controller.set_selected_entity({});

		world_raw raw = {};
		raw.load_from_file(relative_path, editor_settings::get().working_dir.c_str());

		_camera_controller.deactivate();
		world& w = _app.get_world();
		w.create_from_loader(raw, false);
		_camera_controller.activate();

		_loaded_level							   = relative_path;
		editor_settings::get().last_world_relative = relative_path;
		editor_settings::get().save_last();
		raw.destroy();
	}

	void editor::save_lavel()
	{
		// if the last saved path doesn't exists refetch it.
		string target_path = editor_settings::get().working_dir + _loaded_level;
		if (!file_system::exists(target_path.c_str()) || file_system::is_directory(target_path.c_str()))
		{
			string path = process::save_file("save level", "stkworld");
			if (path.empty())
				return;
			file_system::fix_path(path);

			if (!editor_settings::get().is_in_work_directory(path))
			{
				SFG_ERR("save path must be inside working directory: {0}", path.c_str());
				return;
			}
			target_path = path;
		}

		// save to target
		world_raw raw = {};
		raw.save_to_file(target_path.c_str(), _app.get_world());
		raw.destroy();

		// assign both editor settings and runtime loaded level
		_loaded_level							   = editor_settings::get().get_relative(target_path);
		editor_settings::get().last_world_relative = _loaded_level;
		editor_settings::get().save_last();
	}

	void editor::new_project(const char* dir)
	{
		world& w = _app.get_world();

		_camera_controller.deactivate();
		w.uninit();

		editor_settings::get().working_dir = string(dir) + "/";
		file_system::fix_path(editor_settings::get().working_dir);
		editor_settings::get().last_world_relative = "";
		editor_settings::get().save_last();

		_loaded_level = "";
		w.init();
		_camera_controller.activate();
	}

	void editor::new_level()
	{
		// check for prompt later.
		if (!_loaded_level.empty())
		{
		}

		world_raw raw{};
		_camera_controller.deactivate();
		world& w = _app.get_world();
		w.create_from_loader(raw, false);
		_camera_controller.activate();
		_loaded_level = "";
	}

	void editor::enter_playmode(bool is_physics)
	{
		world& w = _app.get_world();

		if (w.get_playmode() != play_mode::none)
			return;

		editor_panel_entities* entities = _gui_controller.get_entities();
		if (entities)
		{
			const world_handle selected = entities->get_selected();
			_playmode_selected_index	= selected.is_null() ? NULL_WORLD_ID : selected.index;
			_gui_controller.set_selected_entity({});
		}
		else
		{
			_playmode_selected_index = NULL_WORLD_ID;
		}

		entity_manager&	   em		  = w.get_entity_manager();
		const world_handle cam_entity = _camera_controller.get_entity();

		_playmode_backup.destroy();
		_playmode_backup.fill_from_world(w);

		_playmode_backup.tool_cam_pos = em.get_entity_position(cam_entity);
		_playmode_backup.tool_cam_rot = em.get_entity_rotation(cam_entity);
		_camera_controller.deactivate();
		w.set_playmode(is_physics ? play_mode::physics_only : play_mode::full);

		_app.get_gameplay().on_world_begin(w);
	}

	void editor::exit_playmode()
	{
		world& w = _app.get_world();

		if (w.get_playmode() == play_mode::none)
			return;

		_app.get_gameplay().on_world_end(w);

		w.set_playmode(play_mode::none);

		w.create_from_loader(_playmode_backup, true);
		_playmode_backup.destroy();

		if (_playmode_selected_index != NULL_WORLD_ID)
		{
			entity_manager& em	   = w.get_entity_manager();
			world_handle	handle = em.get_valid_handle_by_index(_playmode_selected_index);
			if (em.is_valid(handle))
				_gui_controller.set_selected_entity(handle);
			else
				_gui_controller.set_selected_entity({});
		}

		_playmode_selected_index = NULL_WORLD_ID;

		_camera_controller.activate();
	}

	const char* editor::on_vekt_allocate_text(void* ud, size_t sz)
	{
		editor* ed = static_cast<editor*>(ud);
		return ed->_text_allocator.allocate(sz);
	}

	void editor::on_vekt_deallocate_text(void* ud, const char* ptr)
	{
		editor* ed = static_cast<editor*>(ud);
		ed->_text_allocator.deallocate(ptr);
	}

}
