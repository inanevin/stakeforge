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

#include "app/engine_resources.hpp"
#include "app/app.hpp"
#include "resources/shader_raw.hpp"
#include "resources/material_raw.hpp"
#include "resources/font_raw.hpp"
#include "resources/texture_raw.hpp"
#include "resources/common_resources.hpp"

#include "platform/time.hpp"
#include "gfx/renderer.hpp"

#ifdef SFG_TOOLMODE
#include "editor/editor_settings.hpp"
#include "io/file_system.hpp"
#else
#include "app/package_manager.hpp"
#endif

namespace SFG
{
	bool engine_resources::init(app* p)
	{
		SFG_INFO("initializing...");
		const u64 init_begin_time = time::get_cpu_microseconds();

		_app = p;

		const gfx_id bind_layout_global	 = renderer::get_bind_layout_global();
		const gfx_id bind_layout_compute = renderer::get_bind_layout_global_compute();

		_resources = {{
						  .path			= "assets/engine/shaders/debug_controller/console_draw.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_debug_console,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/gui/gui_default.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_gui_default,
						  .store_direct = true,
					  },

					  {
						  .path			= "assets/engine/shaders/gui/gui_sdf.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_gui_sdf,
						  .store_direct = true,
					  },

					  {
						  .path			= "assets/engine/shaders/gui/gui_text.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_gui_text,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/gui/gui_texture.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_gui_texture,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/swapchain/swapchain.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_swapchain,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/lighting/deferred_lighting.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_lighting,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/ssao/hbao.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_hbao,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/ssao/hbao_upsample.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_hbao_upsample,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/bloom/bloom_downsample.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_bloom_downsample,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/bloom/bloom_upsample.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_bloom_upsample,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/post_combiner/post_combiner.stkshader",
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_post_combiner,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/selection/selection_outline_write.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_object_outline_write,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/debug/debug_default.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_debug_triangle,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/debug/debug_line.stkshader",
						  .extra_data	= bind_layout_global,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_debug_line,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/particle/particle_clear.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_particle_clear,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/particle/particle_simulate.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_particle_sim,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/particle/particle_emit.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_particle_emit,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/particle/particle_write_count.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_particle_write_count,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/particle/particle_count.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_particle_count,
						  .store_direct = true,
					  },
					  {
						  .path			= "assets/engine/shaders/particle/particle_swap.stkshader",
						  .extra_data	= bind_layout_compute,
						  .res_type		= engine_resource_type::shader,
						  .ident		= engine_resource_ident::shader_particle_swap,
						  .store_direct = true,
					  },
					  {
						  .path				   = DEFAULT_OPAQUE_SHADER_PATH,
						  .extra_data		   = bind_layout_compute,
						  .res_type			   = engine_resource_type::shader,
						  .ident			   = engine_resource_ident::shader_world_gbuffer,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path				   = DEFAULT_FORWARD_SHADER_PATH,
						  .extra_data		   = bind_layout_compute,
						  .res_type			   = engine_resource_type::shader,
						  .ident			   = engine_resource_ident::shader_world_forward,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path				   = DEFAULT_GUI_SHADER_PATH,
						  .extra_data		   = bind_layout_compute,
						  .res_type			   = engine_resource_type::shader,
						  .ident			   = engine_resource_ident::shader_world_gui_default,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path				   = DEFAULT_GUI_TEXT_SHADER_PATH,
						  .extra_data		   = bind_layout_compute,
						  .res_type			   = engine_resource_type::shader,
						  .ident			   = engine_resource_ident::shader_world_gui_text,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path				   = DEFAULT_GUI_SDF_SHADER_PATH,
						  .extra_data		   = bind_layout_compute,
						  .res_type			   = engine_resource_type::shader,
						  .ident			   = engine_resource_ident::shader_world_gui_sdf,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path				   = DEFAULT_GUI_MAT_PATH,
						  .res_type			   = engine_resource_type::material,
						  .ident			   = engine_resource_ident::mat_world_gui_default,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path				   = DEFAULT_GUI_TEXT_MAT_PATH,
						  .res_type			   = engine_resource_type::material,
						  .ident			   = engine_resource_ident::mat_world_gui_text,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path				   = DEFAULT_GUI_SDF_MAT_PATH,
						  .res_type			   = engine_resource_type::material,
						  .ident			   = engine_resource_ident::mat_world_gui_sdf,
						  .keep_raw_persistent = true,
					  },
					  {
						  .path		= "assets/engine/fonts/roboto.stkfont",
						  .res_type = engine_resource_type::font,
						  .ident	= engine_resource_ident::font_text_default,
					  },
					  {
						  .path		= "assets/engine/fonts/vt323.stkfont",
						  .res_type = engine_resource_type::font,
						  .ident	= engine_resource_ident::font_debug_console_default,
					  },
					  {
						  .path		= "assets/engine/fonts/icons.stkfont",
						  .res_type = engine_resource_type::font,
						  .ident	= engine_resource_ident::font_debug_console_icons,
					  }};

#ifdef SFG_TOOLMODE
		const string res_cache = editor_settings::get()._resource_cache;
		if (!file_system::exists(res_cache.c_str()))
			file_system::create_directory(res_cache.c_str());

		const string root = SFG_ROOT_DIRECTORY;

		auto load = [&](auto* raw, const char* path) -> bool {
			if (!raw->load_from_cache(res_cache.c_str(), path, ".stkcache"))
			{
				if (!raw->load_from_file(path, SFG_ROOT_DIRECTORY))
				{
					return false;
				}
				raw->save_to_cache(res_cache.c_str(), SFG_ROOT_DIRECTORY, ".stkcache");
				return true;
			}

			return true;
		};

		for (engine_resource_def& def : _resources)
		{
			def.sid = TO_SID(def.path);

			const u16 type_u16 = static_cast<u16>(def.ident);

			if (def.res_type == engine_resource_type::shader)
			{
				shader_raw* raw = new shader_raw();

				if (!load(raw, def.path))
				{
					raw->destroy();
					delete raw;
					return false;
				}

				const string src_shader = root + def.path;
				const string src_source = root + raw->source;
				_file_watcher.add_path(src_shader.c_str(), type_u16);
				_file_watcher.add_path(src_source.c_str(), type_u16);
				def.raw = raw;
			}
			else if (def.res_type == engine_resource_type::material)
			{
				material_raw* raw = new material_raw();
				if (!load(raw, def.path))
				{
					raw->destroy();
					delete raw;
					return false;
				}

				const string src = root + def.path;
				_file_watcher.add_path(src.c_str(), type_u16);
				def.raw = raw;
			}
			else if (def.res_type == engine_resource_type::font)
			{
				font_raw* raw = new font_raw();
				if (!load(raw, def.path))
				{
					delete raw;
					return false;
				}

				const string src_shader = root + def.path;
				const string src_source = root + raw->source;
				_file_watcher.add_path(src_shader.c_str(), type_u16);
				_file_watcher.add_path(src_source.c_str(), type_u16);
				def.raw = raw;
			}
			else if (def.res_type == engine_resource_type::texture)
			{
				texture_raw* raw = new texture_raw();
				if (!load(raw, def.path))
				{
					delete raw;
					return false;
				}

				const string src_shader = root + def.path;
				const string src_source = root + raw->source;
				_file_watcher.add_path(src_shader.c_str(), type_u16);
				_file_watcher.add_path(src_source.c_str(), type_u16);
				def.raw = raw;
			}
		}

		_file_watcher.set_callback(on_resource_reloaded, this);
		_file_watcher.set_tick_interval(60);

#else
		package& engine_data_pack = package_manager::get().open_package_engine_data();

		auto load = [&](auto* raw, const char* path) {
			istream stream;
			if (engine_data_pack.get_stream(path, stream))
				raw->deserialize(stream);
		};

		for (engine_resource_def& def : _resources)
		{
			const engine_resource_type type = def.res_type;
			def.sid							= TO_SID(def.path);

			const u16 type_u16 = static_cast<u16>(type);

			if (def.res_type == engine_resource_type::shader)
			{
				shader_raw* raw = new shader_raw();
				load(raw, def.path);
				def.raw = raw;
			}
			else if (def.res_type == engine_resource_type::material)
			{
				material_raw* raw = new material_raw();
				load(raw, def.path);
				def.raw = raw;
			}
			else if (def.res_type == engine_resource_type::font)
			{
				font_raw* raw = new font_raw();
				load(raw, def.path);
				def.raw = raw;
			}
			else if (def.res_type == engine_resource_type::texture)
			{
				texture_raw* raw = new texture_raw();
				load(raw, def.path);
				def.raw = raw;
			}
		}

		engine_data_pack.close();
#endif

		for (engine_resource_def& def : _resources)
		{
			if (!def.store_direct)
				continue;

			if (def.res_type == engine_resource_type::shader)
			{
				shader_direct* dir = new shader_direct();
				shader_raw*	   raw = reinterpret_cast<shader_raw*>(def.raw);
				dir->create_from_loader(*raw, def.extra_data);
				def.direct = dir;
			}
			else
			{
				SFG_ASSERT(false);
			}
		}

		SFG_INFO("initializing took {0} ms", static_cast<double>(time::get_cpu_microseconds() - init_begin_time) / 1000.0);

		return true;
	}

	void engine_resources::clear_init()
	{
		for (engine_resource_def& def : _resources)
		{
			if (def.keep_raw_persistent)
				continue;

			if (def.res_type == engine_resource_type::shader)
			{
				shader_raw* raw = reinterpret_cast<shader_raw*>(def.raw);
				raw->destroy();
				delete raw;
			}
			else if (def.res_type == engine_resource_type::material)
			{
				material_raw* raw = reinterpret_cast<material_raw*>(def.raw);
				raw->destroy();
				delete raw;
			}
			else if (def.res_type == engine_resource_type::texture)
			{
				texture_raw* raw = reinterpret_cast<texture_raw*>(def.raw);
				delete raw;
			}
			else if (def.res_type == engine_resource_type::font)
			{
				font_raw* raw = reinterpret_cast<font_raw*>(def.raw);
				delete raw;
			}
			def.raw = nullptr;
		}
	}

	void engine_resources::uninit()
	{
		for (engine_resource_def& def : _resources)
		{
			if (def.direct != nullptr)
			{
				if (def.res_type == engine_resource_type::shader)
				{
					shader_direct* dir = reinterpret_cast<shader_direct*>(def.direct);
					dir->destroy();
					delete dir;
					def.direct = nullptr;
				}
				else
				{
					SFG_ASSERT(false);
				}
			}

			if (def.raw == nullptr)
				continue;

			if (def.res_type == engine_resource_type::shader)
			{
				shader_raw* raw = reinterpret_cast<shader_raw*>(def.raw);
				raw->destroy();
				delete raw;
			}
			else if (def.res_type == engine_resource_type::material)
			{
				material_raw* raw = reinterpret_cast<material_raw*>(def.raw);
				raw->destroy();
				delete raw;
			}
			else if (def.res_type == engine_resource_type::texture)
			{
				texture_raw* raw = reinterpret_cast<texture_raw*>(def.raw);
				delete raw;
			}
			else if (def.res_type == engine_resource_type::font)
			{
				font_raw* raw = reinterpret_cast<font_raw*>(def.raw);
				delete raw;
			}
			def.raw = nullptr;
		}
	}

	shader_direct& engine_resources::get_shader_direct(engine_resource_ident ident)
	{
		const engine_resource_def& def = _resources.at(ident);
		SFG_ASSERT(def.direct != nullptr && def.res_type == engine_resource_type::shader);
		shader_direct* dir = reinterpret_cast<shader_direct*>(def.direct);
		return *dir;
	}

	shader_raw& engine_resources::get_shader_raw(engine_resource_ident ident)
	{
		const engine_resource_def& def = _resources.at(ident);
		SFG_ASSERT(def.raw != nullptr && def.res_type == engine_resource_type::shader);
		shader_raw* raw = reinterpret_cast<shader_raw*>(def.raw);
		return *raw;
	}

	material_raw& engine_resources::get_material_raw(engine_resource_ident ident)
	{
		const engine_resource_def& def = _resources.at(ident);
		SFG_ASSERT(def.raw != nullptr && def.res_type == engine_resource_type::material);
		material_raw* raw = reinterpret_cast<material_raw*>(def.raw);
		return *raw;
	}

#ifdef SFG_TOOLMODE
	void engine_resources::tick()
	{
		_file_watcher.tick();
	}

	void engine_resources::add_shader_reload_listener(shader_reload_callback&& cb)
	{
		_shader_reload_callbacks.push_back(std::move(cb));
	}

	void engine_resources::on_resource_reloaded(const char* p, u64 last_modified, u16 id, void* user_data)
	{
		engine_resources* self = static_cast<engine_resources*>(user_data);

		const engine_resource_ident ident = static_cast<engine_resource_ident>(id);
		auto						it	  = std::find_if(self->_resources.begin(), self->_resources.end(), [ident](const engine_resource_def& def) -> bool { return def.ident == ident; });
		if (it == self->_resources.end())
			return;

		engine_resource_def& def = *it;
		if (def.direct == nullptr)
			return;

		if (def.res_type == engine_resource_type::shader)
		{
			shader_raw* raw = new shader_raw();
			if (!raw->load_from_file(def.path, SFG_ROOT_DIRECTORY))
				return;

			const string cache = editor_settings::get()._resource_cache;
			raw->save_to_cache(cache.c_str(), SFG_ROOT_DIRECTORY, ".stkcache");

			self->_app->join_render();

			shader_direct* dir = reinterpret_cast<shader_direct*>(def.direct);
			dir->destroy();
			dir->create_from_loader(*raw, def.extra_data);

			if (def.keep_raw_persistent)
			{
				shader_raw* old_raw = reinterpret_cast<shader_raw*>(def.raw);
				old_raw->destroy();
				delete old_raw;

				def.raw = raw;
			}
			else
			{
				raw->destroy();
				delete raw;
			}

			for (auto cb : self->_shader_reload_callbacks)
				cb(ident, *dir);

			self->_app->kick_off_render();
		}
	}

#endif

	const engine_resource_def& engine_resources::get_def(engine_resource_ident t)
	{
		auto it = std::find_if(_resources.begin(), _resources.end(), [t](const engine_resource_def& def) -> bool { return def.ident == t; });
		if (it == _resources.end())
			return _resources[0];

		return *it;
	}

}
