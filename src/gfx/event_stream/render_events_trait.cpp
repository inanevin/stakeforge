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

#include "render_events_trait.hpp"
#include "data/ostream_vector.hpp"
#include "data/istream_vector.hpp"

namespace SFG
{
	void render_event_mesh_instance::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << mesh;
		stream << skin;
		stream << materials;
		stream << skin_node_entities;
	}

	void render_event_mesh_instance::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> mesh;
		stream >> skin;
		stream >> materials;
		stream >> skin_node_entities;
	}

	void render_event_camera::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << near_plane;
		stream << far_plane;
		stream << fov_degrees;
		stream << static_cast<u32>(cascades.size());
		for (f32 f : cascades)
			stream << f;
	}

	void render_event_camera::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> near_plane;
		stream >> far_plane;
		stream >> fov_degrees;
		u32 cascade_count = 0;
		stream >> cascade_count;
		for (u32 i = 0; i < cascade_count; i++)
		{
			f32 f = 0.0f;
			stream >> f;
			cascades.push_back(f);
		}
	}

	void render_event_ambient::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << base_color;
	}

	void render_event_ambient::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> base_color;
	}

	void render_event_bloom::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << filter_radius;
	}

	void render_event_bloom::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> filter_radius;
	}

	void render_event_ssao::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << radius_world;
		stream << bias;
		stream << intensity;
		stream << power;
		stream << num_dirs;
		stream << num_steps;
		stream << random_rot_strength;
	}

	void render_event_ssao::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> radius_world;
		stream >> bias;
		stream >> intensity;
		stream >> power;
		stream >> num_dirs;
		stream >> num_steps;
		stream >> random_rot_strength;
	}

	void render_event_post_process::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << bloom_strength;
		stream << exposure;
		stream << tonemap_mode;
		stream << saturation;
		stream << wb_temp;
		stream << wb_tint;
		stream << reinhard_white_point;
	}

	void render_event_post_process::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> bloom_strength;
		stream >> exposure;
		stream >> tonemap_mode;
		stream >> saturation;
		stream >> wb_temp;
		stream >> wb_tint;
		stream >> reinhard_white_point;
	}

	void render_event_skybox::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << start_color;
		stream << mid_color;
		stream << end_color;
		stream << fog_color;
		stream << fog_start;
		stream << fog_end;
	}

	void render_event_skybox::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> start_color;
		stream >> mid_color;
		stream >> end_color;
		stream >> fog_color;
		stream >> fog_start;
		stream >> fog_end;
	}

	void render_event_point_light::serialize(ostream& stream) const
	{
		stream << shadow_resolution;
		stream << cast_shadows;
		stream << entity_index;
		stream << base_color;
		stream << range;
		stream << intensity;
		stream << near_plane;
	}

	void render_event_point_light::deserialize(istream& stream)
	{
		stream >> shadow_resolution;
		stream >> cast_shadows;
		stream >> entity_index;
		stream >> base_color;
		stream >> range;
		stream >> intensity;
		stream >> near_plane;
	}

	void render_event_dir_light::serialize(ostream& stream) const
	{
		stream << shadow_resolution;
		stream << cast_shadows;
		stream << entity_index;
		stream << base_color;
		stream << intensity;
		stream << max_cascades;
	}

	void render_event_dir_light::deserialize(istream& stream)
	{
		stream >> shadow_resolution;
		stream >> cast_shadows;
		stream >> entity_index;
		stream >> base_color;
		stream >> intensity;
		stream >> max_cascades;
	}

	void render_event_spot_light::serialize(ostream& stream) const
	{
		stream << shadow_resolution;
		stream << cast_shadows;
		stream << entity_index;
		stream << base_color;
		stream << range;
		stream << intensity;
		stream << inner_cone;
		stream << outer_cone;
		stream << near_plane;
	}

	void render_event_spot_light::deserialize(istream& stream)
	{
		stream >> shadow_resolution;
		stream >> cast_shadows;
		stream >> entity_index;
		stream >> base_color;
		stream >> range;
		stream >> intensity;
		stream >> inner_cone;
		stream >> outer_cone;
		stream >> near_plane;
	}

	void render_event_canvas::serialize(ostream& stream) const
	{
		stream << entity_index;
		stream << is_3d;
		stream << vertex_size;
		stream << index_size;
	}

	void render_event_canvas::deserialize(istream& stream)
	{
		stream >> entity_index;
		stream >> is_3d;
		stream >> vertex_size;
		stream >> index_size;
	}

	void render_event_canvas_add_draw::serialize(ostream& stream) const
	{
		stream << clip;
		stream << start_index;
		stream << start_vertex;
		stream << index_count;
		stream << material_handle;
		stream << atlas_handle;
		stream << atlas_exists;
		stream << vertex_data_size;
		stream << index_data_size;

		stream.write_raw(vertex_data, vertex_data_size);
		stream.write_raw(index_data, index_data_size);
	}

	void render_event_canvas_add_draw::deserialize(istream& stream)
	{
		stream >> clip;
		stream >> start_index;
		stream >> start_vertex;
		stream >> index_count;
		stream >> material_handle;
		stream >> atlas_handle;
		stream >> atlas_exists;
		stream >> vertex_data_size;
		stream >> index_data_size;

		vertex_data = stream.get_data_current();
		stream.skip_by(vertex_data_size);
		index_data = stream.get_data_current();
		stream.skip_by(index_data_size);
	}

	void render_event_particle_emitter::serialize(ostream& stream) const
	{
		stream << material;
		stream << particle_res;
		stream << entity;
	}

	void render_event_particle_emitter::deserialize(istream& stream)
	{
		stream >> material;
		stream >> particle_res;
		stream >> entity;
	}

	void render_event_sprite::serialize(ostream& stream) const
	{
		stream << material;
		stream << entity;
	}

	void render_event_sprite::deserialize(istream& stream)
	{
		stream >> material;
		stream >> entity;
	}

	void render_event_mesh_instance_material::serialize(ostream& stream) const
	{
		stream << material;
		stream << index;
	}

	void render_event_mesh_instance_material::deserialize(istream& stream)
	{
		stream >> material;
		stream >> index;
	}

}
