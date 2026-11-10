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

#include "core/runtime/ui/ui_renderer.hpp"
#include "core/runtime/ui/vg/vg_canvas.hpp"
#include "core/gfx/backend/backend.hpp"
#include "core/gfx/common/commands.hpp"
#include "core/gfx/common/descriptions.hpp"
#include "core/gfx/util/gfx_util.hpp"
#include "core/io/assert.hpp"
#include "core/io/log.hpp"
#include "core/math/mat4x4.hpp"
#include "core/runtime/render/render_resources.hpp"
#include "core/runtime/resources/resource_manager.hpp"
#include <tracy/Tracy.hpp>

namespace sfg::ui
{
	void ui_renderer_t::init(const ui_renderer_config_t& cfg)
	{
		SFG_ASSERT(cfg.vertex_buffer_max_bytes >= sizeof(vg_vertex_t) && cfg.index_buffer_max_bytes >= sizeof(vg_index_t));

		_canvas_data_capacity = cfg.world_canvas_max;

		gfx_backend& backend = gfx_backend::get();
		_vtx_capacity		 = cfg.vertex_buffer_max_bytes;
		_idx_capacity		 = cfg.index_buffer_max_bytes;
		_parameter_capacity	 = _vtx_capacity / sizeof(vg_vertex_t);

		for (per_frame_data_t& p : _pfd)
		{
			resource_desc_t vertex_desc{
				.size  = _vtx_capacity,
				.flags = resource_flags::rf_vertex_buffer | resource_flags::rf_cpu_visible,
			};
			vertex_desc.set_name("ui_vertices");
			p.vertex_buffer = backend.create_resource(vertex_desc);
			backend.map_resource(p.vertex_buffer, p.mapped_vtx);

			resource_desc_t index_desc{
				.size  = _idx_capacity,
				.flags = resource_flags::rf_index_buffer | resource_flags::rf_cpu_visible,
			};
			index_desc.set_name("ui_indices");
			p.index_buffer = backend.create_resource(index_desc);
			backend.map_resource(p.index_buffer, p.mapped_idx);

			resource_desc_t projection_desc{
				.size  = sizeof(mat4x4_t),
				.flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
			};
			projection_desc.set_name("ui_projection");
			p.projection_buffer = backend.create_resource(projection_desc);
			backend.map_resource(p.projection_buffer, p.mapped_projection);
			p.projection_index = backend.get_resource_gpu_index(p.projection_buffer);

			resource_desc_t parameter_desc{
				.size			 = _parameter_capacity * static_cast<u32>(sizeof(ui_draw_parameters_gpu_t)),
				.structure_size	 = sizeof(ui_draw_parameters_gpu_t),
				.structure_count = _parameter_capacity,
				.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible,
			};
			parameter_desc.set_name("ui_parameters");
			p.parameter_buffer = backend.create_resource(parameter_desc);
			backend.map_resource(p.parameter_buffer, p.mapped_parameters);
			p.parameter_index = backend.get_resource_gpu_index(p.parameter_buffer);

			if (_canvas_data_capacity != 0)
			{
				resource_desc_t canvas_data_desc{
					.size			 = _canvas_data_capacity * static_cast<u32>(sizeof(ui_canvas_data_t)),
					.structure_size	 = sizeof(ui_canvas_data_t),
					.structure_count = _canvas_data_capacity,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_cpu_visible,
				};
				canvas_data_desc.set_name("ui_canvas_data");
				p.canvas_data_buffer = backend.create_resource(canvas_data_desc);
				backend.map_resource(p.canvas_data_buffer, p.mapped_canvas_data);
				p.canvas_data_index = backend.get_resource_gpu_index(p.canvas_data_buffer);
			}
		}
	}

	void ui_renderer_t::uninit()
	{
		gfx_backend& backend = gfx_backend::get();

		for (per_frame_data_t& p : _pfd)
		{
			backend.destroy_resource(p.vertex_buffer);
			backend.destroy_resource(p.index_buffer);
			backend.destroy_resource(p.projection_buffer);
			backend.destroy_resource(p.parameter_buffer);

			if (_canvas_data_capacity != 0)
				backend.destroy_resource(p.canvas_data_buffer);

			p = {};
		}

		_vtx_capacity		= 0;
		_idx_capacity		= 0;
		_parameter_capacity = 0;
	}

	void ui_renderer_t::render(gfx_handle_t cmd, const vg_draw_snapshot_t* snap, u8 frame_index, vec2u16_t fb_size, const ui_world_render_data_t& world_data)
	{
		ZoneScoped;

		if (snap == nullptr || snap->batch_count == 0)
			return;

		if (upload(*snap, frame_index, fb_size, world_data))
			draw(cmd, snap->batches, snap->batch_count, frame_index, fb_size, world_data);
	}

	bool ui_renderer_t::upload(const vg_draw_snapshot_t& snap, u8 frame_index, vec2u16_t fb_size, const ui_world_render_data_t& world_data)
	{
		ZoneScoped;

		if (snap.vertex_count > _vtx_capacity / sizeof(vg_vertex_t) || snap.index_count > _idx_capacity / sizeof(vg_index_t) || snap.parameter_count > _parameter_capacity)
		{
			SFG_ERR("UI snapshot exceeds upload capacity");
			return false;
		}

		render_resources_t& resources  = render_resources_t::get();
		const u8			frame_slot = frame_index % BACK_BUFFER_COUNT;
		per_frame_data_t&	pfd		   = _pfd[frame_slot];
		const mat4x4_t		projection = mat4x4_t::ortho(0.0f, static_cast<f32>(fb_size.x), 0.0f, static_cast<f32>(fb_size.y), 0.0f, 1.0f);

		SFG_MEMCPY(pfd.mapped_projection, projection.m, sizeof(projection));
		SFG_MEMCPY(pfd.mapped_vtx, snap.vertices, snap.vertex_count * sizeof(vg_vertex_t));
		SFG_MEMCPY(pfd.mapped_idx, snap.indices, snap.index_count * sizeof(vg_index_t));

		if (world_data.canvas_data_count != 0)
		{
			SFG_ASSERT(world_data.canvas_data_count <= _canvas_data_capacity);
			SFG_MEMCPY(pfd.mapped_canvas_data, world_data.canvas_data, world_data.canvas_data_count * sizeof(ui_canvas_data_t));
		}

		const gpu_index_t atlas_index = resources.get_texture_gpu_index(resource_manager_t::get().get_glyph_atlas().get_texture(), 0);

		for (u32 i = 0; i < snap.parameter_count; ++i)
		{
			const ui_draw_parameters_t& source = snap.parameters[i];
			ui_draw_parameters_gpu_t	parameters{.clip = source.clip, .canvas_index = source.canvas_index};

			for (u32 j = 0; j < 4; ++j)
			{
				const ui_resolved_resource_ref_t& ref = source.constants[j];

				switch (ref.type)
				{
				case ui_resource_type_e::gpu_index:
				case ui_resource_type_e::uint:
					parameters.constants[j] = ref.gpu_indices[0];
					break;
				case ui_resource_type_e::gpu_index_fof:
					parameters.constants[j] = ref.gpu_indices[frame_slot];
					break;
				case ui_resource_type_e::texture:
				case ui_resource_type_e::sprite:
					parameters.constants[j] = resources.get_texture_gpu_index(ref.texture, 0);
					break;
				case ui_resource_type_e::glyph_atlas:
					parameters.constants[j] = atlas_index;
					break;
				case ui_resource_type_e::evictable_texture:
					parameters.constants[j] = resources.get_optional_texture_gpu_index(ref.texture, 0);

					if (parameters.constants[j] == NULL_GPU_INDEX)
						parameters.clip = {1.0f, 1.0f, -1.0f, -1.0f};

					break;
				case ui_resource_type_e::none:
					break;
				}
			}

			SFG_MEMCPY(pfd.mapped_parameters + i * sizeof(parameters), &parameters, sizeof(parameters));
		}

		return true;
	}

	void ui_renderer_t::draw(gfx_handle_t cmd, const vg_draw_batch_t* batches, u32 batch_count, u8 frame_index, vec2u16_t fb_size, const ui_world_render_data_t& world_data)
	{
		ZoneScoped;

		gfx_backend&			backend	  = gfx_backend::get();
		render_resources_t&		resources = render_resources_t::get();
		const per_frame_data_t& pfd		  = _pfd[frame_index % BACK_BUFFER_COUNT];

		const gpu_index_t			   constants[] = {pfd.projection_index, pfd.parameter_index, pfd.canvas_data_index, world_data.view_index, world_data.entity_index};
		const command_bind_constants_t bind_constants{.data = constants, .offset = constant_rp0, .count = 5, .param_index = 0};
		backend.cmd_bind_constants(cmd, bind_constants);

		const command_set_scissors_t scissors{.x = 0, .y = 0, .width = fb_size.x, .height = fb_size.y};
		backend.cmd_set_scissors(cmd, scissors);

		const command_bind_vertex_buffers_t vertices{.buffer = pfd.vertex_buffer, .slot = 0, .vertex_size = sizeof(vg_vertex_t), .offset = 0};
		backend.cmd_bind_vertex_buffers(cmd, vertices);

		const command_bind_index_buffers_t indices{.buffer = pfd.index_buffer, .offset = 0, .index_size = sizeof(vg_index_t)};
		backend.cmd_bind_index_buffers(cmd, indices);

		for (u32 i = 0; i < batch_count; ++i)
		{
			const vg_draw_batch_t&		  batch = batches[i];
			const command_bind_pipeline_t pipeline{.pipeline = resources.get_shader_hw(batch.pipeline)};
			backend.cmd_bind_pipeline(cmd, pipeline);

			const command_draw_indexed_instanced_t draw{
				.index_count_per_instance = batch.index_count,
				.instance_count			  = 1,
				.start_index_location	  = batch.index_offset,
				.base_vertex_location	  = 0,
				.start_instance_location  = 0,
			};
			backend.cmd_draw_indexed_instanced(cmd, draw);
		}
	}
}
