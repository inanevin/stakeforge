/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "stock_world_renderer_snapshot.hpp"
#include "world_render_config.hpp"
#include "core/runtime/world/world_debug_draw_config.hpp"
#include "core/runtime/render/render_resources.hpp"

namespace sfg
{
	namespace
	{
		template <bool retain> void update_snapshot_resources(const stock_world_renderer_snapshot_t& snapshot)
		{
			render_resources_t& resources	= render_resources_t::get();
			constexpr auto		resource_op = retain ? &render_resources_t::retain_resource : &render_resources_t::release_resource;
			constexpr auto		texture_op	= retain ? &render_resources_t::retain_texture : &render_resources_t::release_texture;
			constexpr auto		sampler_op	= retain ? &render_resources_t::retain_sampler : &render_resources_t::release_sampler;
			constexpr auto		shader_op	= retain ? &render_resources_t::retain_shader : &render_resources_t::release_shader;
			constexpr auto		canvas_op	= retain ? &ui::vg_draw_snapshot_t::retain_resources : &ui::vg_draw_snapshot_t::release_resources;

			for (const stock_world_renderer_material_t& material : snapshot.materials)
			{
				for (const render_resource_handle_t buffer : material.material_buffers)
				{
					if (!buffer.is_null())
						(resources.*resource_op)(buffer);
				}

				for (u32 i = 0; i < material.texture_count; ++i)
				{
					(resources.*texture_op)(material.material_textures[i]);
					(resources.*sampler_op)(material.material_samplers[i]);
				}

				for (u32 i = 0; i < material.pso_count; ++i)
					(resources.*shader_op)(material.psos[i]);
			}

			for (const stock_world_renderer_mesh_draw_t& draw : snapshot.mesh_draws)
			{
				(resources.*resource_op)(draw.vertex_buffer);
				(resources.*resource_op)(draw.index_buffer);

				if (!draw.direct_pso.is_null())
					(resources.*shader_op)(draw.direct_pso);
			}

			for (const stock_world_renderer_sprite_draw_t& draw : snapshot.sprite_draws)
				(resources.*texture_op)(draw.texture);

			for (const stock_world_renderer_light_t& light : snapshot.lights)
			{
				if (!light.cookie_texture.is_null())
					(resources.*texture_op)(light.cookie_texture);
			}

			for (const world_debug_texture_t& texture : snapshot.debug_draw.textures)
				(resources.*texture_op)(texture.texture);

			(snapshot.canvas.before_post_process.get_snapshot().*canvas_op)();
			(snapshot.canvas.after_post_process.get_snapshot().*canvas_op)();
		}
	}

	void stock_world_renderer_snapshot_t::init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config)
	{
		const u32	 sprite_count		 = std::min(config.sprite_max, config.entity_max);
		const u32	 particle_draw_count = std::min(config.particle_max, config.entity_max);
		const size_t renderable_count	 = static_cast<size_t>(config.mesh_draw_max) + sprite_count + particle_draw_count;

		SFG_ASSERT(renderable_count <= UINT32_MAX);

		materials.init(config.material_max);
		entities.init(config.entity_max);
		bones.init(config.bone_max);
		lights.init(config.light_max);
		reflection_probes.init(config.reflection_probe_max);
		renderables.init(renderable_count);
		mesh_draws.init(config.mesh_draw_max);
		sprite_draws.init(sprite_count);
		particle_draws.init(particle_draw_count);
		particles.init(config.particle_max);

		debug_draw.init(debug_draw_config);
		canvas.init(config);
	}

	void stock_world_renderer_snapshot_t::uninit()
	{
		release_resources();

		materials.uninit();
		entities.uninit();
		bones.uninit();
		lights.uninit();
		reflection_probes.uninit();
		renderables.uninit();
		mesh_draws.uninit();
		sprite_draws.uninit();
		particle_draws.uninit();
		particles.uninit();

		debug_draw.uninit();
		canvas.uninit();

		shadows		  = {};
		main_view	  = {};
		environment	  = {};
		fog			  = {};
		post_process  = {};
		quality_level = engine_quality_level_e::high;
	}

	void stock_world_renderer_snapshot_t::retain_resources()
	{
		SFG_ASSERT(!resources_retained);

		update_snapshot_resources<true>(*this);
		resources_retained = true;
	}

	void stock_world_renderer_snapshot_t::release_resources()
	{
		if (!resources_retained)
			return;

		update_snapshot_resources<false>(*this);
		resources_retained = false;
	}
}
