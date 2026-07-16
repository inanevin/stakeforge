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

#include "world/editor_world_render_context.hpp"
#include "assets/editor_asset.hpp"
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/shader.hpp>

namespace sfg
{
	namespace
	{
		editor_world_gizmo_mesh_t load_gizmo_mesh_render_data(sid_t guid)
		{
			const mesh_internals_t* mesh = resource_manager_t::get().find_internals<mesh_internals_t>(guid);
			SFG_ASSERT(mesh != nullptr);
			return {
				.vertex_buffer = mesh->vertex_buffer,
				.index_buffer  = mesh->index_buffer,
				.index_count   = mesh->index_count,
				.vertex_stride = static_cast<u16>(mesh->vertex_stride),
				.index_stride  = static_cast<u8>(sizeof(primitive_index)),
			};
		}
	}

	void editor_world_render_context_t::init(const world_render_context_config_t& config)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(config.size.x > 0 && config.size.y > 0);

		_world_render_context.init(config);

		resource_desc_t composite_data_desc = {};
		composite_data_desc.size			= static_cast<u32>(sizeof(editor_world_composite_data_t));
		composite_data_desc.flags			= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		composite_data_desc.set_name("editor_world_composite_data");

		resource_desc_t gizmo_data_desc = {};
		gizmo_data_desc.size			= static_cast<u32>(sizeof(editor_world_gizmo_gpu_data_t));
		gizmo_data_desc.flags			= resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
		gizmo_data_desc.set_name("editor_world_gizmo_data");

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			_pfd[i].cmd_gfx = backend.create_command_buffer({
				.type		= command_type::graphics,
				.debug_name = "editor_world",
			});

			_pfd[i].composite_data = backend.create_resource(composite_data_desc);
			backend.map_resource(_pfd[i].composite_data, _pfd[i].mapped_composite_data);
			_pfd[i].composite_data_index = backend.get_resource_gpu_index(_pfd[i].composite_data);

			_pfd[i].gizmo_data = backend.create_resource(gizmo_data_desc);
			backend.map_resource(_pfd[i].gizmo_data, _pfd[i].mapped_gizmo_data);
			_pfd[i].gizmo_data_index = backend.get_resource_gpu_index(_pfd[i].gizmo_data);
		}

		const shader_internals_t* composite_shader = resource_manager_t::get().find_internals<shader_internals_t>("editor/resource_pack/shaders/editor_world_render_texture.hlsl"_hs);
		const shader_internals_t* gizmo_shader	   = resource_manager_t::get().find_internals<shader_internals_t>("editor/resource_pack/shaders/editor_world_gizmo.hlsl"_hs);
		_composite_shader						   = render_resources_t::get().get_shader_hw(composite_shader->psos[0]);
		_gizmo_shader							   = render_resources_t::get().get_shader_hw(gizmo_shader->psos[0]);
		_gizmo_meshes[0]						   = load_gizmo_mesh_render_data(GIZMO_MESH_TRANSLATION);
		_gizmo_meshes[1]						   = load_gizmo_mesh_render_data(GIZMO_MESH_ROTATION);
		_gizmo_meshes[2]						   = load_gizmo_mesh_render_data(GIZMO_MESH_SCALE);
		_gizmo_central_meshes[0]				   = load_gizmo_mesh_render_data(DEFAULT_MESH_SPHERE_GUID);
		_gizmo_central_meshes[1]				   = load_gizmo_mesh_render_data(DEFAULT_MESH_CUBE_GUID);

		create_texture(config.size);
	}

	void editor_world_render_context_t::uninit()
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());

		destroy_texture();
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			backend.destroy_resource(_pfd[i].composite_data);
			backend.destroy_resource(_pfd[i].gizmo_data);
			backend.destroy_command_buffer(_pfd[i].cmd_gfx);
			_pfd[i].cmd_gfx				  = {};
			_pfd[i].composite_data		  = {};
			_pfd[i].gizmo_data			  = {};
			_pfd[i].mapped_composite_data = nullptr;
			_pfd[i].mapped_gizmo_data	  = nullptr;
			_pfd[i].composite_data_index  = NULL_GPU_INDEX;
			_pfd[i].gizmo_data_index	  = NULL_GPU_INDEX;
		}
		for (editor_world_gizmo_mesh_t& mesh : _gizmo_meshes)
			mesh = {};
		for (editor_world_gizmo_mesh_t& mesh : _gizmo_central_meshes)
			mesh = {};
		_composite_shader = {};
		_gizmo_shader	  = {};
		_world_render_context.uninit();
	}

	void editor_world_render_context_t::resize(vec2u16_t size)
	{
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(size.x > 0 && size.y > 0);

		_world_render_context.resize(size);
		destroy_texture();
		create_texture(size);
	}

	entity_id_t editor_world_render_context_t::get_object_id(u8 frame_index, vec2u16_t pixel) const
	{
		SFG_ASSERT(pixel.x < _size.x && pixel.y < _size.y);
		const u8* pixel_data = _pfd[frame_index].mapped_object_id_readback + _object_id_readback_row_pitch * pixel.y + sizeof(entity_id_t) * pixel.x;
		return *reinterpret_cast<const entity_id_t*>(pixel_data);
	}

	void editor_world_render_context_t::create_texture(vec2u16_t size)
	{
		texture_desc_t desc = {};
		desc.texture_format = format_e::r8g8b8a8_srgb;
		desc.size			= size;
		desc.flags			= texture_flags::tf_render_target | texture_flags::tf_sampled | texture_flags::tf_is_2d;
		desc.view_count		= 2;
		desc.views[0]		= {.type = view_type::render_target};
		desc.views[1]		= {.type = view_type::sampled};
		desc.set_name("editor_world_texture");

		texture_desc_t selection_desc  = desc;
		selection_desc.texture_format  = format_e::r8g8b8a8_unorm;
		selection_desc.clear_values[0] = 0.0f;
		selection_desc.clear_values[1] = 0.0f;
		selection_desc.clear_values[2] = 0.0f;
		selection_desc.clear_values[3] = 0.0f;
		selection_desc.set_name("editor_world_selection");

		texture_desc_t object_id_desc  = {};
		object_id_desc.clear_values[0] = NULL_ENTITY_ID;
		object_id_desc.clear_values[1] = 0.0f;
		object_id_desc.clear_values[2] = 0.0f;
		object_id_desc.clear_values[3] = 0.0f;
		object_id_desc.texture_format  = format_e::r32_uint;
		object_id_desc.size			   = size;
		object_id_desc.flags		   = texture_flags::tf_render_target | texture_flags::tf_transfer_source | texture_flags::tf_is_2d;
		object_id_desc.view_count	   = 1;
		object_id_desc.views[0]		   = {.type = view_type::render_target};
		object_id_desc.set_name("editor_world_object_id");

		_object_id_readback_row_pitch = gfx_backend::align_texture_size_pitch(static_cast<u32>(size.x) * sizeof(entity_id_t));

		resource_desc_t object_id_readback_desc = {};
		object_id_readback_desc.size			= _object_id_readback_row_pitch * static_cast<u32>(size.y);
		object_id_readback_desc.flags			= resource_flags::rf_readback;
		object_id_readback_desc.set_name("editor_world_object_id_readback");

		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(_pfd[i].world_texture.is_null());
			SFG_ASSERT(_pfd[i].selection_texture.is_null());
			SFG_ASSERT(_pfd[i].object_id_texture.is_null());
			SFG_ASSERT(_pfd[i].object_id_readback.is_null());

			_pfd[i].world_texture	   = backend.create_texture(desc);
			_pfd[i].selection_texture  = backend.create_texture(selection_desc);
			_pfd[i].object_id_texture  = backend.create_texture(object_id_desc);
			_pfd[i].object_id_readback = backend.create_resource(object_id_readback_desc);
			backend.map_resource(_pfd[i].object_id_readback, _pfd[i].mapped_object_id_readback);
			_pfd[i].world_texture_index		= backend.get_texture_gpu_index(_pfd[i].world_texture, 1);
			_pfd[i].selection_texture_index = backend.get_texture_gpu_index(_pfd[i].selection_texture, 1);
		}
		_size = size;
	}

	void editor_world_render_context_t::destroy_texture()
	{
		gfx_backend& backend = gfx_backend::get();
		for (u32 i = 0; i < BACK_BUFFER_COUNT; ++i)
		{
			SFG_ASSERT(!_pfd[i].world_texture.is_null());
			SFG_ASSERT(!_pfd[i].selection_texture.is_null());
			SFG_ASSERT(!_pfd[i].object_id_texture.is_null());
			SFG_ASSERT(!_pfd[i].object_id_readback.is_null());

			backend.destroy_texture(_pfd[i].world_texture);
			backend.destroy_texture(_pfd[i].selection_texture);
			backend.destroy_texture(_pfd[i].object_id_texture);
			backend.destroy_resource(_pfd[i].object_id_readback);
			_pfd[i].world_texture			  = {};
			_pfd[i].selection_texture		  = {};
			_pfd[i].object_id_texture		  = {};
			_pfd[i].object_id_readback		  = {};
			_pfd[i].mapped_object_id_readback = nullptr;
			_pfd[i].world_texture_index		  = NULL_GPU_INDEX;
			_pfd[i].selection_texture_index	  = NULL_GPU_INDEX;
		}
		_object_id_readback_row_pitch = 0;
		_size						  = vec2u16_t::zero;
	}
}
