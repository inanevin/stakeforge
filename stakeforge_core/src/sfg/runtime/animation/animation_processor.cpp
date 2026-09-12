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

#include "animation_processor.hpp"
#include <sfg/math/math.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/animation_library.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

namespace sfg
{
	namespace
	{

	}

	void animation_processor_t::init(world_t& world, size_t aux_size, size_t max_library_support)
	{
		_world = &world;
		_aux.init(aux_size);
	}

	void animation_processor_t::uninit()
	{
		_world = nullptr;
		_aux.uninit();
	}

	void animation_processor_t::tick(f32 dt)
	{
		ecs_component_table_t& table_lib		   = _world->get_component_table(type_id_t<component_animation_library_t>::value);
		ecs_component_table_t& table_sys_lib	   = _world->get_component_table(type_id_t<component_system_animation_library_t>::value);
		ecs_component_table_t& table_disabled	   = _world->get_component_table(type_id_t<component_disabled_t>::value);
		ecs_component_table_t& table_sys_transform = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		// allocate & deallocate system animation library
		{
			frame_vector_t<entity_id_t> add_sys_entities	= {};
			frame_vector_t<entity_id_t> remove_sys_entities = {};

			// missing sys
			{
				ecs_component_table_ref_t refs[] = {table_lib.ref(), !table_sys_lib.ref(), !table_disabled.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					add_sys_entities.push_back(row.id);
			}

			// remove sys
			{
				ecs_component_table_ref_t refs[] = {!table_lib.ref(), table_sys_lib.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					remove_sys_entities.push_back(row.id);
			}

			// changed
			{
				ecs_component_table_ref_t refs[] = {table_lib.ref(), table_sys_lib.ref(), !table_disabled.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					const component_animation_library_t&		lib		= row.get<component_animation_library_t>();
					const component_system_animation_library_t& sys_lib = row.get<component_system_animation_library_t>();

					if (lib.animation_library == sys_lib.animation_library)
						continue;

					remove_sys_entities.push_back(row.id);
					add_sys_entities.push_back(row.id);
				}
			}

			for (entity_id_t id : remove_sys_entities)
				dealloc_for_entity(id);

			for (entity_id_t id : add_sys_entities)
				alloc_for_entity(id);
		}

		const entity_id_t ent_main_camera = _world->get_main_camera_entity();

		if (ent_main_camera == NULL_ENTITY_ID)
			return;

		const component_camera_t& comp_camera = _world->get_component_table<component_camera_t>().get_as<component_camera_t>(ent_main_camera);

		const float	  fov	  = comp_camera.fov_degrees;
		const vec3f_t cam_pos = _world->get_entity_pos_abs(ent_main_camera);
		const vec3f_t cam_fw  = _world->get_entity_rot_abs(ent_main_camera).get_forward().normalized();

		ecs_component_table_ref_t refs[] = {table_lib.ref(), table_sys_lib.ref(), !table_disabled.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_animation_library_t& lib			   = row.get<component_animation_library_t>();
			const vec3f_t						 pos			   = _world->get_entity_pos_abs(row.id);
			const vec3f_t						 cam_to_pos		   = (pos - cam_pos).normalized();
			const float							 dot			   = vec3f_t::dot(cam_to_pos, cam_fw);
			const bool							 process_animation = dot < math::cos(lib.cull_angle_limit * DEG_2_RAD);

			// process the library.
		}
	}

	void animation_processor_t::alloc_for_entity(entity_id_t id)
	{
		// const component_animation_library_t&  lib = _world->get_component_table<component_animation_library_t>().get_as<component_animation_library_t>(id);
		// component_system_animation_library_t& sys = _world->get_component_table<component_system_animation_library_t>().get_as<component_system_animation_library_t>(id);
		//
		// sys.lib_alloc = _aux.allocate<animator_library_t>(1);
		//
		// animator_library_t* animator_library = _aux.get<animator_library_t>(sys.lib_alloc);
		// SFG_FAIL(animator_library, "could not allocate animator library!");
		//
		// resource_manager_t&				   rm			   = resource_manager_t::get();
		// const animation_library_runtime_t* library_runtime = rm.find_runtime<animation_library_runtime_t>(lib.animation_library);
	}

	void animation_processor_t::dealloc_for_entity(entity_id_t id)
	{
		// const component_animation_library_t&		lib = _world->get_component_table<component_animation_library_t>().get_as<component_animation_library_t>(id);
		// const component_system_animation_library_t& sys = _world->get_component_table<component_system_animation_library_t>().get_as<component_system_animation_library_t>(id);
	}
}
