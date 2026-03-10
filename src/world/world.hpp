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

#include "data/bitmask.hpp"
#include "data/vector.hpp"
#include "memory/text_allocator.hpp"

// world
#include "world/entity_manager.hpp"
#include "world/component_manager.hpp"
#include "world/common_world.hpp"
#include "world/time_manager.hpp"
#include "world/world_debug_rendering.hpp"
#include "world/world_screen.hpp"

#include "resources/resource_manager.hpp"
#include "physics/physics_world.hpp"
#include "audio/audio_manager.hpp"

// animation
#include "animation/animation_graph.hpp"

#include "gui/vekt_defines.hpp"
struct ma_engine;

namespace vekt
{
	class atlas;
	class font_manager;
}

namespace SFG
{
	struct window_event;
	struct vector2ui16;
	struct world_raw;
	class render_event_stream;
	class window;

	class world
	{
	public:
		enum flags
		{
			world_flags_is_init = 1 << 0,
		};

	public:
		world(render_event_stream& rstream);
		~world();

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void init_preserve_resources();
		void uninit_preserve_resources();
		void create_from_loader(world_raw& raw, bool preserve_resources);
		void tick(const vector2ui16& res, f32 dt);
		void begin_debug_tick(const vector2ui16& res);
		void end_debug_tick();
		void calculate_abs_transforms();

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		void interpolate(double interpolation);
		void set_prev_transforms();
#endif
		bool on_window_event(const window_event& ev, window* wnd);

		// -----------------------------------------------------------------------------
		// playmode
		// -----------------------------------------------------------------------------
		void set_playmode(play_mode mode);

		// -----------------------------------------------------------------------------
		// atlas
		// -----------------------------------------------------------------------------

		resource_handle find_atlas_texture(vekt::id atl);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline play_mode get_playmode() const
		{
			return _play_mode;
		}

		inline world_debug_rendering& get_debug_rendering()
		{
			return _debug_rendering;
		}

		inline world_screen& get_screen()
		{
			return _screen;
		}

		inline physics_world& get_physics_world()
		{
			return _phy_world;
		}

		inline entity_manager& get_entity_manager()
		{
			return _entity_manager;
		}

		inline component_manager& get_comp_manager()
		{
			return _comp_manager;
		}

		inline bitmask<u8>& get_flags()
		{
			return _flags;
		}

		inline text_allocator& get_text_allocator()
		{
			return _text_allocator;
		}

		inline vekt::font_manager& get_font_manager()
		{
			return *_vekt_fonts;
		}

		inline render_event_stream& get_render_stream()
		{
			return _render_stream;
		}

		inline resource_manager& get_resource_manager()
		{
			return _resource_manager;
		}

		inline audio_manager& get_audio_manager()
		{
			return _audio_manager;
		}

		inline animation_graph& get_animation_graph()
		{
			return _anim_graph;
		}

		inline time_manager& get_time_manager()
		{
			return _time_manager;
		}

#ifdef SFG_TOOLMODE

		inline void set_tool_camera_pos(const vector3& p)
		{
			_tool_camera_pos = p;
		}

		inline void set_tool_camera_rot(const quat& q)
		{
			_tool_camera_rot = q;
		}

		inline const vector3& get_tool_camera_pos() const
		{
			return _tool_camera_pos;
		}

		inline const quat& get_tool_camera_rot() const
		{
			return _tool_camera_rot;
		}
#endif
	private:
		// -----------------------------------------------------------------------------
		// vekt
		// -----------------------------------------------------------------------------

		struct atlas_data
		{
			vekt::atlas*	atlas  = nullptr;
			resource_handle handle = {};
		};

		static void on_atlas_created(vekt::atlas* atlas, void* user_data);
		static void on_atlas_updated(vekt::atlas* atlas, void* user_data);
		static void on_atlas_destroyed(vekt::atlas* atlas, void* user_data);

	private:
		vekt::font_manager* _vekt_fonts = nullptr;

		resource_manager	  _resource_manager;
		entity_manager		  _entity_manager;
		physics_world		  _phy_world;
		component_manager	  _comp_manager;
		text_allocator		  _text_allocator;
		audio_manager		  _audio_manager   = {};
		animation_graph		  _anim_graph	   = {};
		time_manager		  _time_manager	   = {};
		world_debug_rendering _debug_rendering = {};
		world_screen		  _screen		   = {};

		vector<atlas_data>	 _vekt_atlases = {};
		render_event_stream& _render_stream;

		bitmask<u8> _flags	   = 0;
		play_mode	_play_mode = play_mode::none;

#ifdef SFG_TOOLMODE
		vector3 _tool_camera_pos = vector3::zero;
		quat	_tool_camera_rot = quat::identity;
#endif
	};
}
