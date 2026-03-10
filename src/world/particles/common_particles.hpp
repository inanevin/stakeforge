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
#include "common/size_definitions.hpp"
#include "math/vector4.hpp"
#include "math/vector3.hpp"
#include "math/vector2.hpp"
#include "math/color.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{
	class ostream;
	class istream;

	struct particle_spawn_settings
	{
		f32 emitter_lifetime   = 0.0f;
		f32 wait_between_emits = 0.0f;
		f32 min_lifetime	   = 1.0f;
		f32 max_lifetime	   = 1.0f;
		u32 min_particle_count = 1;
		u32 max_particle_count = 1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct particle_position_settings
	{
		vector3 min_start	= vector3::zero;
		vector3 max_start	= vector3::zero;
		f32		cone_radius = 0.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct particle_velocity_settings
	{
		vector3 min_start		= vector3::zero;
		vector3 max_start		= vector3::zero;
		vector3 min_mid			= vector3::zero;
		vector3 max_mid			= vector3::zero;
		vector3 min_end			= vector3::zero;
		vector3 max_end			= vector3::zero;
		f32		integrate_point = -1;
		u8		is_local		= 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct particle_color_settings
	{
		vector3 min_start			  = vector3::one;
		vector3 max_start			  = vector3::one;
		vector3 mid_color			  = vector3::one;
		vector3 end_color			  = vector3::one;
		f32		integrate_point_color = 0.0f;

		f32 min_start_opacity		= 1.0f;
		f32 max_start_opacity		= 1.0f;
		f32 mid_opacity				= 1.0f;
		f32 end_opacity				= 1.0f;
		f32 integrate_point_opacity = -1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct particle_rotation_settings
	{
		f32 min_start_rotation				 = 0.0f;
		f32 max_start_rotation				 = 0.0f;
		f32 min_start_angular_velocity		 = 0.0f;
		f32 max_start_angular_velocity		 = 0.0f;
		f32 min_end_angular_velocity		 = 0.0f;
		f32 max_end_angular_velocity		 = 0.0f;
		f32 integrate_point_angular_velocity = -1;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct particle_size_settings
	{
		f32 min_start		= 1.0f;
		f32 max_start		= 1.0f;
		f32 mid				= 1.0f;
		f32 end				= 1.0f;
		f32 integrate_point = -1.0f;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct particle_emit_properties
	{
		particle_spawn_settings	   spawn		  = {};
		particle_position_settings pos			  = {};
		particle_velocity_settings velocity		  = {};
		particle_rotation_settings rotation		  = {};
		particle_size_settings	   size			  = {};
		particle_color_settings	   color_settings = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const particle_spawn_settings& s);
	void from_json(const nlohmann::json& j, particle_spawn_settings& s);

	void to_json(nlohmann::json& j, const particle_position_settings& s);
	void from_json(const nlohmann::json& j, particle_position_settings& s);

	void to_json(nlohmann::json& j, const particle_velocity_settings& s);
	void from_json(const nlohmann::json& j, particle_velocity_settings& s);

	void to_json(nlohmann::json& j, const particle_rotation_settings& s);
	void from_json(const nlohmann::json& j, particle_rotation_settings& s);

	void to_json(nlohmann::json& j, const particle_size_settings& s);
	void from_json(const nlohmann::json& j, particle_size_settings& s);

	void to_json(nlohmann::json& j, const particle_color_settings& s);
	void from_json(const nlohmann::json& j, particle_color_settings& s);

	void to_json(nlohmann::json& j, const particle_emit_properties& s);
	void from_json(const nlohmann::json& j, particle_emit_properties& s);

#endif
}
