// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/world/ecs_component_type.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
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
		// tables
		// -----------------------------------------------------------------------------

		world_component_table_t&	   add_component_table(const ecs_component_type_desc_t& desc);
		const world_component_table_t* find_component_table(sid_t type_id) const;
		world_component_table_t*	   find_component_table(sid_t type_id);
		world_component_table_t*	   get_component_table(sid_t type_id);
		bool						   is_alive(entity_id_t id) const;

	private:
		vector_t<world_component_table_t> _component_tables;
		vector_t<entity_id_t>			  _entity_free_list;
		ecs_component_table_t*			  _component_hierarchy_table	 = nullptr;
		ecs_component_table_t*			  _component_transform_table	 = nullptr;
		ecs_component_table_t*			  _component_mesh_renderer_table = nullptr;
		ecs_component_table_t*			  _component_alive_table		 = nullptr;
		ecs_component_table_t*			  _component_disabled_table		 = nullptr;
		entity_id_t						  _entity_head					 = 0;
	};

}
