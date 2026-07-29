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

#include "game.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/platform/time.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/engine/engine_runtime.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/scripting/api/script_api_game.hpp>
#include <sfg/runtime/scripting/api/script_api_platform.hpp>
#include <sfg/runtime/scripting/script_runtime.hpp>
#include <sfg/serialization/serialization.hpp>
#include <tracy/Tracy.hpp>

namespace sfg
{
#define GAME_RAW_WHEEL_DELTA 120.0f

	void game_t::on_window_event(void* window_handle, const window_event_t& event, void* user_data)
	{
		game_t& game = *static_cast<game_t*>(user_data);

		if (event.type == window_event_type_e::resize)
		{
			if (game._renderer_initialized)
				game._renderer.resize(game._window.size, game._window.monitor_info.dpi_scale, game._window.has_flag(window_runtime_flags_e::minimized));

			return;
		}

		if (!game._world_controller_initialized || !game._world_controller.get_main_world().is_playing())
			return;

		world_t& world = game._world_controller.get_main_world();

		switch (event.type)
		{
		case window_event_type_e::key:
			if (game._window.has_flag(window_runtime_flags_e::has_focus))
				world.key_event(event.button, static_cast<u16>(event.value.x), static_cast<u8>(event.sub_type));
			break;
		case window_event_type_e::mouse: {
			u8 button = UINT8_MAX;

			if (event.button == static_cast<u16>(input_code::mouse_0))
				button = 0;
			else if (event.button == static_cast<u16>(input_code::mouse_1))
				button = 1;
			else if (event.button == static_cast<u16>(input_code::mouse_2))
				button = 2;
			else if (event.button == static_cast<u16>(input_code::mouse_3))
				button = 3;
			else if (event.button == static_cast<u16>(input_code::mouse_4))
				button = 4;

			world.mouse_button_event(button, static_cast<u8>(event.sub_type), static_cast<f32>(game._window.mouse_position.x), static_cast<f32>(game._window.mouse_position.y));
			break;
		}
		case window_event_type_e::delta:
			world.mouse_move_event(static_cast<f32>(game._window.mouse_position.x), static_cast<f32>(game._window.mouse_position.y), static_cast<f32>(event.value.x), static_cast<f32>(event.value.y));
			break;
		case window_event_type_e::wheel: {
			const f32 delta = event.flags.is_set(static_cast<u8>(wef_high_freq)) ? static_cast<f32>(event.value.y) / GAME_RAW_WHEEL_DELTA : static_cast<f32>(event.value.y);

			world.mouse_wheel_event(static_cast<f32>(game._window.mouse_position.x), static_cast<f32>(game._window.mouse_position.y), delta);
			break;
		}
		default:
			break;
		}
	}

	u8 game_t::get_script_game_render_resolution(vec2u16_t& out_resolution)
	{
		SFG_ASSERT(s_instance != nullptr);

		if (!s_instance->_world_controller.is_initialized())
			return 0;

		out_resolution = s_instance->_world_controller.get_render_resolution();
		return 1;
	}

	u8 game_t::set_script_game_render_resolution(const vec2u16_t& resolution)
	{
		SFG_ASSERT(s_instance != nullptr);

		if (!s_instance->_world_controller.is_initialized())
			return 0;

		if (s_instance->_world_controller.get_render_resolution() == resolution)
			return 1;

		const bool restart_render = SFG_IS_RENDER_RUNNING();

		if (restart_render)
			s_instance->_renderer.end_render();

		s_instance->_world_controller.resize(resolution);

		if (restart_render)
			s_instance->_renderer.start();

		return 1;
	}

	u8 game_t::load_script_game_world(sid_t world_name_hash)
	{
		SFG_ASSERT(s_instance != nullptr);

		if (!s_instance->_world_controller.is_initialized())
			return 0;

		return s_instance->_world_controller.queue_world_load_by_name_hash(world_name_hash) ? 1 : 0;
	}

	void game_t::quit_script_game()
	{
		SFG_ASSERT(s_instance != nullptr);

		s_instance->_window.set_flag(window_runtime_flags_e::close_requested);
	}

	bool game_t::init(const game_config_t& config)
	{
		SFG_ASSERT(config.main_frame_budget_bytes != 0);
		SFG_ASSERT(config.renderer.frame_budget_bytes != 0);
		SFG_ASSERT(!_initialized);
		SFG_ASSERT(s_instance == nullptr);

		s_instance = this;
		_config	   = config;
		_init_failure_reason.resize(0);
		_package_directory = file_system_t::get_running_directory();

		const string_t resource_path = _package_directory + project_package_meta_t::RESOURCE_FILE_NAME;
		const string_t meta_path	 = _package_directory + project_package_meta_t::FILE_NAME;

		if (!file_system_t::exists(resource_path.c_str()))
			return fail_init("resources.sfg_bin is missing.");

		if (!file_system_t::exists(meta_path.c_str()))
			return fail_init("project_meta.sfg_bin is missing.");

		istream_t meta_stream = serializer_t::load_from_file(meta_path.c_str());

		if (meta_stream.empty())
			return fail_init("Failed to read project_meta.sfg_bin.");

		if (!_package_meta.deserialize(meta_stream))
			return fail_init("project_meta.sfg_bin has an incompatible cook version or is corrupt.");

		engine_runtime_t& runtime = engine_runtime_t::get();

		runtime.init_globals(config.engine.global);
		_globals_initialized = true;

		if (!runtime.init_backend(config.engine.backend))
			return fail_init("Failed to initialize the graphics backend.");

		_backend_initialized = true;

		if (!runtime.init())
			return fail_init("Failed to initialize the engine runtime.");

		_runtime_initialized = true;
		runtime.update_project_settings(_package_meta.project_settings);

		if (!load_project_scripts())
			return false;

		frame_allocator_tls_t::init(config.main_frame_budget_bytes);
		_frame_allocator_initialized = true;

		if (!load_package_resources())
			return false;

		render_resources_t::get().drain_requests();

		vector_t<monitor_info_t> monitors = {};
		process::get_all_monitors(monitors);
		SFG_ASSERT(!monitors.empty());

		const monitor_info_t& monitor		  = process::find_primary_monitor(monitors);
		const vec2u16_t		  window_size	  = _package_meta.is_fullscreen ? monitor.size : _package_meta.window_resolution;
		const vec2i16_t		  window_position = _package_meta.is_fullscreen ? monitor.position
																			: vec2i16_t{
																				  static_cast<i16>(monitor.position.x + (static_cast<i32>(monitor.work_size.x) - static_cast<i32>(window_size.x)) / 2),
																				  static_cast<i16>(monitor.position.y + (static_cast<i32>(monitor.work_size.y) - static_cast<i32>(window_size.y)) / 2),
																			  };
		const window_style_e  window_style	  = _package_meta.is_fullscreen ? window_style_e::borderless : _package_meta.window_style;

		if (!process::create_window("Stakeforge Game", window_position, window_size, window_style, 1.0f, false, _window))
			return fail_init("Failed to create the game window.");

		_window_initialized				 = true;
		_window.event_callback			 = &game_t::on_window_event;
		_window.event_callback_user_data = this;

		g_window_api_enabled = true;
		set_script_api_platform_window_runtime(&_window);
		set_script_api_game_callbacks(get_script_game_render_resolution, set_script_game_render_resolution, load_script_game_world, quit_script_game);
		_script_api_bound = true;

		if (!_world_controller.init(_window.size, _package_meta))
			return fail_init("Failed to load the main world.");

		_world_controller_initialized = true;

		game_renderer_config_t renderer_config = config.renderer;
		renderer_config.is_fullscreen		   = _package_meta.is_fullscreen;

		if (!_renderer.init(_window, _world_controller, renderer_config))
			return fail_init("Failed to initialize the game renderer.");

		_renderer_initialized = true;
		_initialized		  = true;

		return true;
	}

	void game_t::uninit()
	{
		SFG_ASSERT(_initialized);

		cleanup();
		_init_failure_reason.resize(0);
	}

	void game_t::run()
	{
		SFG_ASSERT(_initialized);

		_renderer.start();

#ifdef TRACY_ENABLE
		tracy::SetThreadName("main");
#endif

		while (!_window.has_flag(window_runtime_flags_e::close_requested))
		{
			frame_allocator_tls_t::reset();
			process::pump_os_messages();

			if (_window.has_flag(window_runtime_flags_e::close_requested))
				break;

			resource_manager_t::get().flush();
			_world_controller.tick();

			if (!apply_pending_world_load())
				SFG_ERR("failed to load the requested game world.");

			engine_runtime_t::get().tick();
			resource_manager_t::get().drain_atlases(_atlas_frame_slot);

			_atlas_frame_slot = static_cast<u8>((_atlas_frame_slot + 1) % BACK_BUFFER_COUNT);

			FrameMarkNamed("main");
			time_t::yield_thread();
		}

		_renderer.end_render();
	}

	bool game_t::load_package_resources()
	{
		resource_stream_t resource_stream = {};
		const string_t	  resource_path	  = _package_directory + project_package_meta_t::RESOURCE_FILE_NAME;

		if (!resource_stream.open(resource_path.c_str()))
			return fail_init("Failed to open resources.sfg_bin.");

		u8 stream_header_data[sizeof(u32) * 3] = {};

		if (!resource_stream.read_exact(stream_header_data, sizeof(stream_header_data)))
			return fail_init("Failed to read the cooked resource header.");

		istream_t stream_header(stream_header_data, sizeof(stream_header_data));
		u32		  wire_magic	  = 0;
		u32		  wire_version	  = 0;
		u32		  primitive_count = 0;

		stream_header >> wire_magic;
		stream_header >> wire_version;
		stream_header >> primitive_count;

		if (wire_magic != project_package_meta_t::RESOURCE_STREAM_WIRE_MAGIC || wire_version != project_package_meta_t::RESOURCE_STREAM_WIRE_VERSION)
			return fail_init("resources.sfg_bin has an incompatible cook version.");

		resource_manager_t& resource_manager = resource_manager_t::get();

		for (u32 primitive_index = 0; primitive_index < primitive_count; ++primitive_index)
		{
			u8 primitive_header_data[sizeof(sid_t) + sizeof(u64)] = {};

			if (!resource_stream.read_exact(primitive_header_data, sizeof(primitive_header_data)))
				return fail_init("Failed to read a cooked primitive header.");

			istream_t primitive_header(primitive_header_data, sizeof(primitive_header_data));
			sid_t	  primitive_sid	 = NULL_SID;
			u64		  primitive_size = 0;

			primitive_header >> primitive_sid;
			primitive_header >> primitive_size;

			istream_t primitive_stream = {};
			primitive_stream.create(nullptr, static_cast<size_t>(primitive_size));

			if (!resource_stream.read_exact(primitive_stream.get_raw(), primitive_stream.get_size()))
				return fail_init("Failed to read a cooked primitive.");

			if (resource_manager.load_resource_runtime(primitive_sid, resource_type_e::mesh, primitive_stream) == resource_state_e::failed)
				return fail_init("Failed to load a cooked primitive.");
		}

		u8 engine_count_data[sizeof(u32)] = {};

		if (!resource_stream.read_exact(engine_count_data, sizeof(engine_count_data)))
			return fail_init("Failed to read the engine resource count.");

		istream_t engine_count_stream(engine_count_data, sizeof(engine_count_data));
		u32		  engine_resource_count = 0;

		engine_count_stream >> engine_resource_count;

		vector_t<resource_dependency_t> engine_resources = {};
		engine_resources.reserve(engine_resource_count);

		for (u32 resource_index = 0; resource_index < engine_resource_count; ++resource_index)
		{
			u8 resource_header_data[sizeof(sid_t) + sizeof(u8) + sizeof(u64)] = {};

			if (!resource_stream.read_exact(resource_header_data, sizeof(resource_header_data)))
				return fail_init("Failed to read an engine resource header.");

			istream_t		resource_header(resource_header_data, sizeof(resource_header_data));
			sid_t			resource_sid  = NULL_SID;
			resource_type_e resource_type = resource_type_e::invalid;
			u64				resource_size = 0;

			resource_header >> resource_sid;
			resource_header >> resource_type;
			resource_header >> resource_size;

			const resource_map_info_t resource_info{
				.offset = static_cast<size_t>(resource_stream.get_cursor()),
				.size	= static_cast<size_t>(resource_size),
			};
			const bool inserted = _package_meta.resource_map.emplace(resource_sid, resource_info).second;
			SFG_ASSERT(inserted);

			engine_resources.push_back({
				.handle = resource_sid,
				.type	= resource_type,
			});

			const bool seeked = resource_stream.seek(static_cast<i64>(resource_size), resource_seek_origin_e::current);
			SFG_ASSERT(seeked);
		}

		engine_runtime_t::get().get_resource_file_system().set_mode_filepack(resource_path.c_str(), _package_meta.resource_map);

		for (const resource_dependency_t& resource : engine_resources)
		{
			if (resource_manager.load_resource(resource.handle, resource.type) == resource_state_e::failed)
				return fail_init("Failed to load an engine manifest resource.");
		}

		return true;
	}

	bool game_t::load_project_scripts()
	{
		if (_package_meta.script_assembly_name.empty())
			return fail_init("The packaged C# project assembly name is missing.");

		const string_t assembly_path = _package_directory + "scripts/" + _package_meta.script_assembly_name;

		if (!file_system_t::exists(assembly_path.c_str()))
			return fail_init("The packaged C# project assembly is missing.");

		script_runtime_t& script_runtime = script_runtime_t::get();

		if (!script_runtime.stage_project_assembly(assembly_path.c_str()))
			return fail_init("Failed to stage the packaged C# project assembly.");

		if (!script_runtime.activate_staged_project_assembly())
		{
			script_runtime.discard_staged_project_assembly();
			return fail_init("Failed to activate the packaged C# project assembly.");
		}

		reflection_registry_t::get().remove_script_types();
		script_runtime.get_component_schema().register_reflection_types();
		return true;
	}

	bool game_t::apply_pending_world_load()
	{
		if (!_world_controller.has_pending_world_load())
			return true;

		_renderer.end_render();

		const bool loaded = _world_controller.apply_pending_world_load();

		resource_manager_t::get().flush();
		render_resources_t::get().drain_requests();
		_renderer.start();

		return loaded;
	}

	bool game_t::fail_init(const char* reason)
	{
		_init_failure_reason = reason;
		cleanup();
		return false;
	}

	void game_t::cleanup()
	{
		if (_renderer_initialized)
		{
			_renderer.uninit();
			_renderer_initialized = false;
		}

		if (_world_controller_initialized)
		{
			_world_controller.uninit();
			_world_controller_initialized = false;
		}

		if (_script_api_bound)
		{
			reset_script_api_platform_cursor_state();
			set_script_api_platform_window_runtime(nullptr);
			set_script_api_game_callbacks(nullptr, nullptr, nullptr, nullptr);
			_script_api_bound = false;
		}

		if (_window_initialized)
		{
			process::destroy_window(_window.window_handle);
			_window				= {};
			_window_initialized = false;
		}

		engine_runtime_t& runtime = engine_runtime_t::get();

		if (_runtime_initialized)
		{
			runtime.uninit();
			_runtime_initialized = false;
		}

		if (_globals_initialized)
		{
			runtime.uninit_globals();
			_globals_initialized = false;
		}

		if (_backend_initialized)
		{
			runtime.uninit_backend();
			_backend_initialized = false;
		}

		if (_frame_allocator_initialized)
		{
			frame_allocator_tls_t::uninit();
			_frame_allocator_initialized = false;
		}

		_package_meta = {};
		_config		  = {};
		_package_directory.resize(0);
		_atlas_frame_slot = 0;
		_initialized	  = false;
		s_instance		  = nullptr;
	}
}
