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
		world_t()							= default;
		~world_t()							= default;
		world_t(const world_t&)				= delete;
		world_t& operator=(const world_t&)	= delete;
		world_t(world_t&& other)			= delete;
		world_t& operator=(world_t&& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const world_init_config_t& config);
		void uninit();
		void tick(f32 delta_time);

		// -----------------------------------------------------------------------------
		// entity
		// -----------------------------------------------------------------------------

		entity_id_t	  create_entity(const char* name = nullptr, entity_guid_t guid = NULL_ENTITY_GUID);
		void		  destroy_entity(entity_id_t id);
		void		  destroy_entity_tree(entity_id_t id);
		void		  set_entity_name(entity_id_t id, const char* name);
		entity_id_t	  spawn_prefab(resource_handle_t handle, const prefab_spawn_params_t& params);
		void		  make_prefab_chain(entity_id_t root, resource_handle_t handle);
		void		  refresh_prefab_instances(resource_handle_t handle, entity_id_t skip = NULL_ENTITY_ID);
		void		  break_prefab_chain(entity_id_t root);
		entity_id_t	  get_entity_parent(entity_id_t id) const;
		entity_guid_t get_entity_guid(entity_id_t id) const;
		entity_id_t	  find_by_guid(entity_guid_t guid) const;
		entity_id_t	  get_entity_from_guid(entity_guid_t guid) const;
		void		  attach_to(entity_id_t id, entity_id_t parent);
		void		  detach(entity_id_t id);
		void		  sync_entity_hierarchy(entity_id_t id);

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		bool add_resource(resource_type_e type, resource_handle_t handle);
		void scan_for_resources(entity_id_t entity, bool omit_children = false);
		void load_all_used_resources();
		void unload_all_used_resources();

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

	private:
		entity_id_t spawn_prefab(resource_handle_t handle, const prefab_internals_t& prefab_data, const prefab_spawn_params_t& params);
		void		update_entity_transform(entity_id_t id, const component_hierarchy_t& own_hierarchy, const vec3f_t& parent_abs_pos, const quat_t& parent_abs_rot, const vec3f_t& parent_abs_scale, const mat4x3_t& parent_abs_mat, bool advance_interpolation);
		void		set_entity_snap_interpolation_recursive(entity_id_t id);
		mat4x3_t	calculate_parent_transform_direct(entity_id_t id);
		void		refresh_component_table_cache();

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

		struct world_resource_t
		{
			resource_handle_t handle = NULL_RESOURCE_HANDLE;
			resource_type_e	  type	 = resource_type_e::invalid;
			bool			  loaded = false;
		};

	private:
		vector_t<ecs_component_table_t>	  _component_tables;
		vector_t<world_text_allocation_t> _text_allocations;
		vector_t<u32>					  _text_allocation_free_list;
		vector_t<entity_id_t>			  _entity_free_list;
		vector_t<world_resource_t>		  _used_resources;
		text_allocator_t				  _text_allocator;
		engine_components_t				  _engine_components;
		system_components_t				  _system_components;
		entity_id_t						  _entity_head = 0;
	};

}
