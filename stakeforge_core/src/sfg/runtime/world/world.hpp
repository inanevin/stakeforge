// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/vendor/nhlohmann/json_fwd.hpp>

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/world/ecs_component_type.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class mat4x3_t;
	class quat_t;
	struct component_hierarchy_t;
	struct vec3f_t;

	struct world_component_table_t
	{
		ecs_component_type_desc_t type_desc = {};
		ecs_component_table_t	  table		= {};
	};

	class world_t
	{
	public:
		world_t()  = default;
		~world_t() = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void tick(f32 delta_time);

		// -----------------------------------------------------------------------------
		// entity
		// -----------------------------------------------------------------------------

		entity_id_t create_entity();
		void		destroy_entity(entity_id_t id);
		void		attach_to(entity_id_t id, entity_id_t parent);
		void		detach(entity_id_t id);

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

		world_component_table_t&	   add_component_table(const ecs_component_type_desc_t& desc);
		const world_component_table_t* find_component_table(sid_t type_id) const;
		world_component_table_t*	   find_component_table(sid_t type_id);
		world_component_table_t*	   get_component_table(sid_t type_id);
		bool						   is_alive(entity_id_t id) const;

	private:
		struct engine_components_t
		{
			ecs_component_table_t* hierarchy_table	   = nullptr;
			ecs_component_table_t* transform_table	   = nullptr;
			ecs_component_table_t* mesh_renderer_table = nullptr;
			ecs_component_table_t* render_object_table = nullptr;
			ecs_component_table_t* camera_table		   = nullptr;
			ecs_component_table_t* alive_table		   = nullptr;
			ecs_component_table_t* disabled_table	   = nullptr;
			ecs_component_table_t* no_serialize_table  = nullptr;
		};

		struct system_components_t
		{
			ecs_component_table_t* transform_table = nullptr;
		};

		void	 update_entity_transform(entity_id_t id, const component_hierarchy_t& own_hierarchy, const vec3f_t& parent_abs_pos, const quat_t& parent_abs_rot, const vec3f_t& parent_abs_scale, const mat4x3_t& parent_abs_mat, bool advance_interpolation);
		void	 set_entity_snap_interpolation_recursive(entity_id_t id);
		mat4x3_t calculate_parent_transform_direct(entity_id_t id);

		vector_t<world_component_table_t> _component_tables;
		vector_t<entity_id_t>			  _entity_free_list;
		engine_components_t				  _engine_components;
		system_components_t				  _system_components;
		entity_id_t						  _entity_head = 0;
	};

	void to_json(nlohmann::json& j, const world_t& world);
	void from_json(const nlohmann::json& j, world_t& world);
}
