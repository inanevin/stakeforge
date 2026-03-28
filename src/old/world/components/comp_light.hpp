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

#include "world/components/common_comps.hpp"
#include "game/game_max_defines.hpp"
#include "reflection/type_reflection.hpp"
#include "data/static_vector.hpp"
#include "math/color.hpp"
#include "math/vector2ui16.hpp"

namespace SFG
{
#define SFG_LIGHT_CANDELA_MULT 1.0f / 683.0f

	class world;

	class comp_dir_light
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		void set_values(world& w, const color& c, f32 intensity);
		void set_shadow_values(world& w, u8 cast_shadows, u8 max_cascades, const vector2ui16& resolution);

		inline const color& get_color() const
		{
			return _base_color;
		}

		inline f32 get_intensity() const
		{
			return _intensity;
		}
		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void send_event(world& w);

	private:
		component_header _header			= {};
		color			 _base_color		= color::white;
		vector2ui16		 _shadow_resolution = vector2ui16(256, 256);
		f32				 _intensity			= 0.0f;
		bool			 _cast_shadows		= false;
		u8				 _max_cascades		= 1;
	};

	class comp_spot_light
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		void set_values(world& w, const color& c, f32 range, f32 intensity, f32 inner_cone, f32 outer_cone);
		void set_shadow_values(world& w, u8 cast_shadows, f32 near_plane, const vector2ui16& resolution);

		inline const color& get_color() const
		{
			return _base_color;
		}

		inline f32 get_range() const
		{
			return _range;
		}

		inline f32 get_intensity() const
		{
			return _intensity;
		}

		inline f32 get_inner_cone() const
		{
			return _inner_cone;
		}

		inline f32 get_outer_cone() const
		{
			return _outer_cone;
		}

		inline f32 get_near_plane() const
		{
			return _near_plane;
		}
		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void send_event(world& w);

	private:
		component_header _header			= {};
		color			 _base_color		= color::white;
		vector2ui16		 _shadow_resolution = vector2ui16(256, 256);
		f32				 _range				= 0.0f;
		f32				 _intensity			= 0.0f;
		f32				 _inner_cone		= 0.0f;
		f32				 _outer_cone		= 0.0f;
		f32				 _near_plane		= 0.0f;
		bool			 _cast_shadows		= false;
	};

	class comp_point_light
	{
	public:
		static void reflect();

		// -----------------------------------------------------------------------------
		// trait
		// -----------------------------------------------------------------------------

		void on_add(world& w);
		void on_remove(world& w);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		void set_values(world& w, const color& c, f32 range, f32 intensity);
		void set_shadow_values(world& w, u8 cast_shadows, f32 near_plane, const vector2ui16& resolution);

		inline const color& get_color() const
		{
			return _base_color;
		}

		inline f32 get_range() const
		{
			return _range;
		}

		inline f32 get_intensity() const
		{
			return _intensity;
		}

		inline f32 get_near_plane() const
		{
			return _near_plane;
		}
		inline const component_header& get_header() const
		{
			return _header;
		}

	private:
		template <typename T, int> friend class comp_cache;

		void send_event(world& w);

	private:
		component_header _header			= {};
		color			 _base_color		= color::white;
		vector2ui16		 _shadow_resolution = vector2ui16(256, 256);
		f32				 _range				= 0.0f;
		f32				 _intensity			= 0.0f;
		f32				 _near_plane		= 0.0f;
		bool			 _cast_shadows		= false;
	};

	REFLECT_TYPE(comp_dir_light);
	REFLECT_TYPE(comp_spot_light);
	REFLECT_TYPE(comp_point_light);
}
