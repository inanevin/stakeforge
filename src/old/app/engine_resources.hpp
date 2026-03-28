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

#include "common/string_id.hpp"
#include "data/static_vector.hpp"
#include "resources/shader_direct.hpp"

#ifdef SFG_TOOLMODE
#include "io/simple_file_watcher.hpp"
#endif

namespace SFG
{
	enum engine_resource_ident : u8
	{
		shader_debug_console,
		shader_gui_default,
		shader_gui_sdf,
		shader_gui_text,
		shader_gui_texture,
		shader_swapchain,
		shader_lighting,
		shader_hbao,
		shader_hbao_upsample,
		shader_bloom_downsample,
		shader_bloom_upsample,
		shader_post_combiner,
		shader_object_outline_write,
		shader_debug_triangle,
		shader_debug_line,
		shader_particle_clear,
		shader_particle_sim,
		shader_particle_emit,
		shader_particle_write_count,
		shader_particle_count,
		shader_particle_swap,
		shader_world_gbuffer,
		shader_world_forward,
		shader_world_gui_default,
		shader_world_gui_text,
		shader_world_gui_sdf,
		mat_world_gui_default,
		mat_world_gui_sdf,
		mat_world_gui_text,
		font_text_default,
		font_debug_console_default,
		font_debug_console_icons,
		max,
	};

	enum class engine_resource_type
	{
		unknown,
		shader,
		material,
		texture,
		font,
	};

	struct engine_resource_def
	{
		const char*			  path				  = "";
		void*				  raw				  = nullptr;
		void*				  direct			  = nullptr;
		string_id			  sid				  = 0;
		u32					  extra_data		  = 0;
		engine_resource_type  res_type			  = engine_resource_type::unknown;
		engine_resource_ident ident				  = engine_resource_ident::font_debug_console_default;
		bool				  keep_raw_persistent = false;
		bool				  store_direct		  = false;
	};

	class app;
	class shader_direct;
	class shader_raw;
	class material_raw;

	class engine_resources
	{
	public:
		static engine_resources& get()
		{
			static engine_resources res;
			return res;
		}

		bool		   init(app* p);
		void		   clear_init();
		void		   uninit();
		shader_direct& get_shader_direct(engine_resource_ident ident);
		shader_raw&	   get_shader_raw(engine_resource_ident ident);
		material_raw&  get_material_raw(engine_resource_ident ident);

#ifdef SFG_TOOLMODE
		typedef std::function<void(engine_resource_ident ident, shader_direct& sh)> shader_reload_callback;

		void		tick();
		void		add_shader_reload_listener(shader_reload_callback&& cb);
		static void on_resource_reloaded(const char* p, u64 last_modified, u16 id, void* user_data);
#endif

		const engine_resource_def& get_def(engine_resource_ident t);

		inline const static_vector<engine_resource_def, engine_resource_ident::max>& get_resources() const
		{
			return _resources;
		}

	private:
		static_vector<engine_resource_def, engine_resource_ident::max> _resources;
		app*														   _app = nullptr;

#ifdef SFG_TOOLMODE
		static_vector<shader_reload_callback, 100> _shader_reload_callbacks;
		simple_file_watcher						   _file_watcher;
#endif
	};
}
