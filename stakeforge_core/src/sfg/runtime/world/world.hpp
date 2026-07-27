// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/text_allocator.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/resources/resource_type.hpp>
#include <sfg/runtime/world/ecs_component_type.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/world/world_animation_controller.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#include <sfg/runtime/world/world_logic_helper.hpp>
#include <sfg/runtime/world/world_particle_simulation.hpp>
#include <sfg/runtime/physics/physics_world.hpp>

namespace sfg
{
	class mat4x3_t;
	class istream_t;
	struct world_init_config_t;
	struct component_hierarchy_t;
	struct prefab_internals_t;

	struct prefab_spawn_params_t
	{
		entity_id_t parent		= NULL_ENTITY_ID;
		vec3f_t		local_pos	= vec3f_t::zero;
		quat_t		local_rot	= {};
		vec3f_t		local_scale = vec3f_t::one;
	};

	class world_t
	{
	public:
		world_t();
		~world_t();
		world_t(const world_t&)				= delete;
		world_t& operator=(const world_t&)	= delete;
		world_t(world_t&& other)			= delete;
		world_t& operator=(world_t&& other) = delete;

		struct world_resource_t
		{
			resource_handle_t handle = NULL_RESOURCE_HANDLE;
			resource_type_e	  type	 = resource_type_e::invalid;
			bool			  loaded = false;
		};

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const world_init_config_t& config);
		void uninit();
		void begin_play();
		void end_play();
		void clear_entities();
		void tick_physics(f32 dt);
		void tick_animation_prep(f32 dt);
		void tick_animation_logic(f32 dt);
		void tick_logic(f32 dt);
		void tick_post(f32 dt);

		// -----------------------------------------------------------------------------
		// entity
		// -----------------------------------------------------------------------------

		void		  recreate_physical(entity_id_t id);
		entity_guid_t generate_guid() const;
		entity_id_t	  create_entity(const char* name = nullptr, entity_guid_t guid = NULL_ENTITY_GUID);
		void		  destroy_entity(entity_id_t id);
		void		  destroy_entity_tree(entity_id_t id);
		void		  set_entity_name(entity_id_t id, const char* name);
		entity_id_t	  get_entity_parent(entity_id_t id) const;
		entity_guid_t get_entity_guid(entity_id_t id) const;
		entity_id_t	  find_by_guid(entity_guid_t guid) const;
		void		  attach_to(entity_id_t id, entity_id_t parent);
		void		  detach(entity_id_t id);
		void		  sync_entity_hierarchy(entity_id_t id);

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		bool									 add_resource(resource_type_e type, resource_handle_t handle);
		void									 scan_for_resources(entity_id_t entity, bool omit_children = false);
		void									 load_all_used_resources();
		void									 unload_all_used_resources();
		inline const vector_t<world_resource_t>& get_used_resources() const
		{
			return _used_resources;
		}

		// -----------------------------------------------------------------------------
		// transformation
		// -----------------------------------------------------------------------------

		void		   set_entity_pos_local(entity_id_t id, const vec3f_t& pos);
		void		   set_entity_rot_local(entity_id_t id, const quat_t& rot);
		void		   set_entity_scale_local(entity_id_t id, const vec3f_t& scale);
		void		   teleport_entity(entity_id_t id, const vec3f_t& pos, const quat_t& rot, const vec3f_t& scale);
		void		   mark_entity_teleported(entity_id_t id);
		const vec3f_t& get_entity_pos_local(entity_id_t id) const;
		const quat_t&  get_entity_rot_local(entity_id_t id) const;
		const vec3f_t& get_entity_scale_local(entity_id_t id) const;
		const vec3f_t& get_entity_pos_last_abs(entity_id_t id) const;
		const quat_t&  get_entity_rot_last_abs(entity_id_t id) const;
		const vec3f_t& get_entity_scale_last_abs(entity_id_t id) const;
		vec3f_t		   abs_pos_to_local(entity_id_t id, const vec3f_t& pos);
		quat_t		   abs_rot_to_local(entity_id_t id, const quat_t& rot);
		vec3f_t		   abs_scale_to_local(entity_id_t id, const vec3f_t& scale);
		mat4x3_t	   calculate_transform_direct(entity_id_t id);
		void		   update_world_transforms(bool advance_interpolation = true);

		// -----------------------------------------------------------------------------
		// tables
		// -----------------------------------------------------------------------------

		ecs_component_table_t&				   add_component_table(const ecs_component_type_desc_t& desc);
		const ecs_component_table_t*		   find_component_table(sid_t type_id) const;
		ecs_component_table_t*				   find_component_table(sid_t type_id);
		const ecs_component_table_t&		   get_component_table(sid_t type_id) const;
		ecs_component_table_t&				   get_component_table(sid_t type_id);
		const vector_t<ecs_component_table_t>& get_component_tables() const;
		const char*							   get_entity_name(entity_id_t id) const;
		const char*							   get_text(u32 text_index) const;
		u32									   allocate_text(const char* text);
		void								   release_text(u32 text_index);
		bool								   is_alive(entity_id_t id) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline world_debug_draw_t& get_debug_draw()
		{
			return _debug_draw;
		}

		inline const world_debug_draw_t& get_debug_draw() const
		{
			return _debug_draw;
		}

		inline physics_world_t& get_physics()
		{
			return _physics_world;
		}

		inline const physics_world_t& get_physics() const
		{
			return _physics_world;
		}

		inline const world_animation_controller_t& get_animation_controller() const
		{
			return _animation_controller;
		}

		inline world_particle_simulation_t& get_particle_simulation()
		{
			return _particle_simulation;
		}

		inline const world_particle_simulation_t& get_particle_simulation() const
		{
			return _particle_simulation;
		}

		inline entity_id_t get_main_camera_entity() const
		{
			return _main_camera_entity;
		}

	private:
		void	 update_entity_transform(entity_id_t id, const component_hierarchy_t& own_hierarchy, const vec3f_t& parent_abs_pos, const quat_t& parent_abs_rot, const vec3f_t& parent_abs_scale, const mat4x3_t& parent_abs_mat, bool advance_interpolation);
		void	 set_entity_snap_interpolation_recursive(entity_id_t id);
		mat4x3_t calculate_parent_transform_direct(entity_id_t id);

	private:
		struct world_text_allocation_t
		{
			const char* allocated = nullptr;
		};

		struct engine_components_t
		{
			ecs_component_table_t* hierarchy_table = nullptr;
			ecs_component_table_t* guid_table	   = nullptr;
			ecs_component_table_t* transform_table = nullptr;
			ecs_component_table_t* name_table	   = nullptr;
			ecs_component_table_t* alive_table	   = nullptr;
			ecs_component_table_t* prefab_table	   = nullptr;
		};

		struct system_components_t
		{
			ecs_component_table_t* transform_table = nullptr;
		};

	private:
		vector_t<ecs_component_table_t>	  _component_tables;
		vector_t<world_text_allocation_t> _text_allocations;
		vector_t<u32>					  _text_allocation_free_list;
		vector_t<entity_id_t>			  _entity_free_list;
		vector_t<world_resource_t>		  _used_resources;
		world_debug_draw_t				  _debug_draw			= {};
		physics_world_t					  _physics_world		= {};
		world_animation_controller_t	  _animation_controller = {};
		world_logic_helper_t			  _logic_helper			= {};
		world_particle_simulation_t		  _particle_simulation	= {};
		text_allocator_t				  _text_allocator		= {};
		engine_components_t				  _engine_components	= {};
		system_components_t				  _system_components	= {};
		u64								  _tick_count			= 0;
		entity_id_t						  _entity_head			= 0;
		entity_id_t						  _main_camera_entity	= NULL_ENTITY_ID;
		u32								  _play_resource_count	= 0;
		bool							  _is_playing			= false;
	};

}
