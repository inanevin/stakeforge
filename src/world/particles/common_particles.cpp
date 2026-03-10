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

#include "common_particles.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
#endif

namespace SFG
{
	void particle_emit_properties::serialize(ostream& stream) const
	{
		stream << spawn;
		stream << pos;
		stream << velocity;
		stream << rotation;
		stream << size;
		stream << color_settings;
	}

	void particle_emit_properties::deserialize(istream& stream)
	{
		stream >> spawn;
		stream >> pos;
		stream >> velocity;
		stream >> rotation;
		stream >> size;
		stream >> color_settings;
	}

	void particle_spawn_settings::serialize(ostream& stream) const
	{
		stream << emitter_lifetime;
		stream << wait_between_emits;
		stream << min_particle_count;
		stream << max_particle_count;
		stream << min_lifetime;
		stream << max_lifetime;
	}

	void particle_spawn_settings::deserialize(istream& stream)
	{
		stream >> emitter_lifetime;
		stream >> wait_between_emits;
		stream >> min_particle_count;
		stream >> max_particle_count;
		stream >> min_lifetime;
		stream >> max_lifetime;
	}

	void particle_position_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << cone_radius;
	}

	void particle_position_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> cone_radius;
	}

	void particle_velocity_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << min_mid;
		stream << max_mid;
		stream << min_end;
		stream << max_end;
		stream << integrate_point;
		stream << is_local;
	}

	void particle_velocity_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> min_mid;
		stream >> max_mid;
		stream >> min_end;
		stream >> max_end;
		stream >> integrate_point;
		stream >> is_local;
	}
	void particle_color_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << mid_color;
		stream << end_color;
		stream << integrate_point_color;
		stream << min_start_opacity;
		stream << max_start_opacity;
		stream << mid_opacity;
		stream << end_opacity;
		stream << integrate_point_opacity;
	}
	void particle_color_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> mid_color;
		stream >> end_color;
		stream >> integrate_point_color;
		stream >> min_start_opacity;
		stream >> max_start_opacity;
		stream >> mid_opacity;
		stream >> end_opacity;
		stream >> integrate_point_opacity;
	}
	void particle_rotation_settings::serialize(ostream& stream) const
	{
		stream << min_start_rotation;
		stream << max_start_rotation;
		stream << min_start_angular_velocity;
		stream << max_start_angular_velocity;
		stream << min_end_angular_velocity;
		stream << max_end_angular_velocity;
		stream << integrate_point_angular_velocity;
	}
	void particle_rotation_settings::deserialize(istream& stream)
	{
		stream >> min_start_rotation;
		stream >> max_start_rotation;
		stream >> min_start_angular_velocity;
		stream >> max_start_angular_velocity;
		stream >> min_end_angular_velocity;
		stream >> max_end_angular_velocity;
		stream >> integrate_point_angular_velocity;
	}
	void particle_size_settings::serialize(ostream& stream) const
	{
		stream << min_start;
		stream << max_start;
		stream << mid;
		stream << end;
		stream << integrate_point;
	}
	void particle_size_settings::deserialize(istream& stream)
	{
		stream >> min_start;
		stream >> max_start;
		stream >> mid;
		stream >> end;
		stream >> integrate_point;
	}

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const particle_spawn_settings& s)
	{
		j = nlohmann::json{
			{"emitter_lifetime", s.emitter_lifetime},
			{"wait_between_emits", s.wait_between_emits},
			{"min_particle_count", s.min_particle_count},
			{"max_particle_count", s.max_particle_count},
			{"min_lifetime", s.min_lifetime},
			{"max_lifetime", s.max_lifetime},
		};
	}

	void from_json(const nlohmann::json& j, particle_spawn_settings& s)
	{
		s.emitter_lifetime	 = j.value("emitter_lifetime", decltype(s.emitter_lifetime){});
		s.wait_between_emits = j.value("wait_between_emits", decltype(s.wait_between_emits){});
		s.min_particle_count = j.value("min_particle_count", decltype(s.min_particle_count){});
		s.max_particle_count = j.value("max_particle_count", decltype(s.max_particle_count){});
		s.min_lifetime		 = j.value("min_lifetime", decltype(s.min_lifetime){});
		s.max_lifetime		 = j.value("max_lifetime", decltype(s.max_lifetime){});
	}

	// --- particle_position_settings ----------------------------------------------
	void to_json(nlohmann::json& j, const particle_position_settings& s)
	{
		j = nlohmann::json{
			{"min_start", s.min_start},
			{"max_start", s.max_start},
			{"cone_radius", s.cone_radius},
		};
	}

	void from_json(const nlohmann::json& j, particle_position_settings& s)
	{
		s.min_start	  = j.value("min_start", decltype(s.min_start){});
		s.max_start	  = j.value("max_start", decltype(s.max_start){});
		s.cone_radius = j.value("cone_radius", decltype(s.cone_radius){});
	}

	// --- particle_velocity_settings ----------------------------------------------
	void to_json(nlohmann::json& j, const particle_velocity_settings& s)
	{
		j = nlohmann::json{
			{"min_start", s.min_start},
			{"max_start", s.max_start},
			{"min_mid", s.min_mid},
			{"max_mid", s.max_mid},
			{"min_end", s.min_end},
			{"max_end", s.max_end},
			{"integrate_point", s.integrate_point},
			{"is_local", s.is_local},
		};
	}

	void from_json(const nlohmann::json& j, particle_velocity_settings& s)
	{
		s.min_start		  = j.value("min_start", decltype(s.min_start){});
		s.max_start		  = j.value("max_start", decltype(s.max_start){});
		s.min_mid		  = j.value("min_mid", decltype(s.min_mid){});
		s.max_mid		  = j.value("max_mid", decltype(s.max_mid){});
		s.min_end		  = j.value("min_end", decltype(s.min_end){});
		s.max_end		  = j.value("max_end", decltype(s.max_end){});
		s.integrate_point = j.value("integrate_point", decltype(s.integrate_point){});
		s.is_local		  = j.value("is_local", decltype(s.is_local){});
	}

	// --- particle_rotation_settings ----------------------------------------------
	void to_json(nlohmann::json& j, const particle_rotation_settings& s)
	{
		j = nlohmann::json{
			{"min_start_rotation", s.min_start_rotation},
			{"max_start_rotation", s.max_start_rotation},
			{"min_start_angular_velocity", s.min_start_angular_velocity},
			{"max_start_angular_velocity", s.max_start_angular_velocity},
			{"min_end_angular_velocity", s.min_end_angular_velocity},
			{"max_end_angular_velocity", s.max_end_angular_velocity},
			{"integrate_point_angular_velocity", s.integrate_point_angular_velocity},
		};
	}

	void from_json(const nlohmann::json& j, particle_rotation_settings& s)
	{
		s.min_start_rotation			   = j.value("min_start_rotation", decltype(s.min_start_rotation){});
		s.max_start_rotation			   = j.value("max_start_rotation", decltype(s.max_start_rotation){});
		s.min_start_angular_velocity	   = j.value("min_start_angular_velocity", decltype(s.min_start_angular_velocity){});
		s.max_start_angular_velocity	   = j.value("max_start_angular_velocity", decltype(s.max_start_angular_velocity){});
		s.min_end_angular_velocity		   = j.value("min_end_angular_velocity", decltype(s.min_end_angular_velocity){});
		s.max_end_angular_velocity		   = j.value("max_end_angular_velocity", decltype(s.max_end_angular_velocity){});
		s.integrate_point_angular_velocity = j.value("integrate_point_angular_velocity", decltype(s.integrate_point_angular_velocity){});
	}

	// --- particle_size_settings --------------------------------------------------
	void to_json(nlohmann::json& j, const particle_size_settings& s)
	{
		j = nlohmann::json{
			{"min_start", s.min_start},
			{"max_start", s.max_start},
			{"mid", s.mid},
			{"end", s.end},
			{"integrate_point", s.integrate_point},
		};
	}

	void from_json(const nlohmann::json& j, particle_size_settings& s)
	{
		s.min_start		  = j.value("min_start", decltype(s.min_start){});
		s.max_start		  = j.value("max_start", decltype(s.max_start){});
		s.mid			  = j.value("mid", decltype(s.mid){});
		s.end			  = j.value("end", decltype(s.end){});
		s.integrate_point = j.value("integrate_point", decltype(s.integrate_point){});
	}

	// --- particle_color_settings -------------------------------------------------
	void to_json(nlohmann::json& j, const particle_color_settings& s)
	{
		j = nlohmann::json{
			{"min_start", s.min_start},
			{"max_start", s.max_start},
			{"mid_color", s.mid_color},
			{"end_color", s.end_color},
			{"integrate_point_color", s.integrate_point_color},
			{"min_start_opacity", s.min_start_opacity},
			{"max_start_opacity", s.max_start_opacity},
			{"mid_opacity", s.mid_opacity},
			{"end_opacity", s.end_opacity},
			{"integrate_point_opacity", s.integrate_point_opacity},
		};
	}

	void from_json(const nlohmann::json& j, particle_color_settings& s)
	{
		s.min_start				  = j.value<vector3>("min_start", vector3::one);
		s.max_start				  = j.value<vector3>("max_start", vector3::one);
		s.mid_color				  = j.value<vector3>("mid_color", vector3::one);
		s.end_color				  = j.value<vector3>("end_color", vector3::one);
		s.integrate_point_color	  = j.value<f32>("integrate_point_color", 0.0f);
		s.min_start_opacity		  = j.value<f32>("min_start_opacity", 1.0f);
		s.max_start_opacity		  = j.value<f32>("max_start_opacity", 1.0f);
		s.mid_opacity			  = j.value<f32>("mid_opacity", 1.0f);
		s.end_opacity			  = j.value<f32>("end_opacity", 1.0f);
		s.integrate_point_opacity = j.value<f32>("integrate_point_opacity", 0.0f);
	}

	// --- particle_emit_properties ------------------------------------------------
	void to_json(nlohmann::json& j, const particle_emit_properties& s)
	{
		j = nlohmann::json{
			{"spawn", s.spawn},
			{"pos", s.pos},
			{"velocity", s.velocity},
			{"rotation", s.rotation},
			{"size", s.size},
			{"color", s.color_settings},
		};
	}

	void from_json(const nlohmann::json& j, particle_emit_properties& s)
	{
		s.spawn			 = j.value("spawn", particle_spawn_settings{});
		s.pos			 = j.value("pos", particle_position_settings{});
		s.velocity		 = j.value("velocity", particle_velocity_settings{});
		s.rotation		 = j.value("rotation", particle_rotation_settings{});
		s.size			 = j.value("size", particle_size_settings{});
		s.color_settings = j.value("color", particle_color_settings{});
	}

#endif
}
