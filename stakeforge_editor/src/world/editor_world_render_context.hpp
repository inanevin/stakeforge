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

#pragma once

#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct editor_world_composite_data_t
	{
		mat4x4_t proj			  = mat4x4_t::identity;
		mat4x4_t inv_proj		  = mat4x4_t::identity;
		mat4x4_t inv_view		  = mat4x4_t::identity;
		vec4f_t	 camera_position  = vec4f_t::zero;
		vec4f_t	 camera_grid	  = vec4f_t::zero;
		vec4f_t	 grid_params	  = vec4f_t::zero;
		vec4f_t	 grid_minor_color = vec4f_t::zero;
		vec4f_t	 grid_major_color = vec4f_t::zero;
		vec4f_t	 grid_x_color	  = vec4f_t::zero;
		vec4f_t	 grid_z_color	  = vec4f_t::zero;
		vec4f_t	 params			  = vec4f_t::zero;
		vec4f_t	 selection_color  = vec4f_t::zero;
	};

	struct editor_world_gizmo_gpu_data_t
	{
		mat4x4_t models[7]	= {};
		vec4f_t	 colors[7]	= {};
		vec4f_t	 offsets[7] = {};
		vec4f_t	 params		= vec4f_t::zero;
	};

	struct editor_world_gizmo_mesh_t
	{
		render_resource_handle_t vertex_buffer = {};
		render_resource_handle_t index_buffer  = {};
		u32						 index_count   = 0;
		u16						 vertex_stride = 0;
		u8						 index_stride  = 0;
	};

	class editor_world_render_context_t final
	{
	public:
		editor_world_render_context_t()												   = default;
		~editor_world_render_context_t()											   = default;
		editor_world_render_context_t(const editor_world_render_context_t&)			   = delete;
		editor_world_render_context_t& operator=(const editor_world_render_context_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config);
		void uninit();
		void resize(vec2u16_t size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline world_render_context_t& get_world_render_context()
		{
			return _world_render_context;
		}

		inline const world_render_context_t& get_world_render_context() const
		{
			return _world_render_context;
		}

		inline gfx_handle_t get_command_buffer(u8 frame_index) const
		{
			return _pfd[frame_index].cmd_gfx;
		}

		inline gfx_handle_t get_world_texture(u8 frame_index) const
		{
			return _pfd[frame_index].world_texture;
		}

		inline gfx_handle_t get_selection_texture(u8 frame_index) const
		{
			return _pfd[frame_index].selection_texture;
		}

		inline gfx_handle_t get_object_id_texture(u8 frame_index) const
		{
			return _pfd[frame_index].object_id_texture;
		}

		inline gfx_handle_t get_object_id_readback(u8 frame_index) const
		{
			return _pfd[frame_index].object_id_readback;
		}

		inline gpu_index_t get_world_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].world_texture_index;
		}

		inline gpu_index_t get_selection_texture_index(u8 frame_index) const
		{
			return _pfd[frame_index].selection_texture_index;
		}

		inline gpu_index_t get_composite_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].composite_data_index;
		}

		inline gpu_index_t get_gizmo_data_index(u8 frame_index) const
		{
			return _pfd[frame_index].gizmo_data_index;
		}

		inline u8* get_mapped_composite_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_composite_data;
		}

		inline u8* get_mapped_gizmo_data(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_gizmo_data;
		}

		inline u8* get_mapped_object_id_readback(u8 frame_index) const
		{
			return _pfd[frame_index].mapped_object_id_readback;
		}

		entity_id_t get_object_id(u8 frame_index, vec2u16_t pixel) const;

		inline gfx_handle_t get_composite_shader() const
		{
			return _composite_shader;
		}

		inline gfx_handle_t get_gizmo_shader() const
		{
			return _gizmo_shader;
		}

		inline gfx_handle_t get_physics_debug_shader() const
		{
			return _physics_debug_shader;
		}

		inline const editor_world_gizmo_mesh_t& get_gizmo_mesh(u8 index) const
		{
			return _gizmo_meshes[index];
		}

		inline const editor_world_gizmo_mesh_t& get_gizmo_central_mesh(u8 index) const
		{
			return _gizmo_central_meshes[index];
		}

		inline vec2u16_t get_size() const
		{
			return _size;
		}

	private:
		void create_texture(vec2u16_t size);
		void destroy_texture();

		struct per_frame_data_t
		{
			u8*			 mapped_composite_data	   = nullptr;
			u8*			 mapped_gizmo_data		   = nullptr;
			u8*			 mapped_object_id_readback = nullptr;
			gfx_handle_t cmd_gfx				   = {};
			gfx_handle_t world_texture			   = {};
			gfx_handle_t selection_texture		   = {};
			gfx_handle_t object_id_texture		   = {};
			gfx_handle_t object_id_readback		   = {};
			gfx_handle_t composite_data			   = {};
			gfx_handle_t gizmo_data				   = {};
			gpu_index_t	 world_texture_index	   = NULL_GPU_INDEX;
			gpu_index_t	 selection_texture_index   = NULL_GPU_INDEX;
			gpu_index_t	 composite_data_index	   = NULL_GPU_INDEX;
			gpu_index_t	 gizmo_data_index		   = NULL_GPU_INDEX;
		};

	private:
		world_render_context_t _world_render_context = {};

		per_frame_data_t _pfd[BACK_BUFFER_COUNT] = {};

		editor_world_gizmo_mesh_t _gizmo_meshes[3]		   = {};
		editor_world_gizmo_mesh_t _gizmo_central_meshes[2] = {};

		gfx_handle_t _composite_shader	   = {};
		gfx_handle_t _gizmo_shader		   = {};
		gfx_handle_t _physics_debug_shader = {};

		u32		  _object_id_readback_row_pitch = 0;
		vec2u16_t _size							= vec2u16_t::zero;
	};
}
