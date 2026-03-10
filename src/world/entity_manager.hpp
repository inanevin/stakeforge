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

#pragma once
#include "common_world.hpp"
#include "common_entity.hpp"
#include "data/vector.hpp"
#include "memory/static_array.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "memory/chunk_allocator.hpp"
#include "math/aabb.hpp"
#include "math/matrix4x3.hpp"
#include "math/quat.hpp"
#include "game/game_max_defines.hpp"
#include "common/type_id.hpp"
#include "resources/common_resources.hpp"
#include "game/app_defines.hpp"

namespace SFG
{
	class world;
	struct entity_template_raw;

	struct entity_transform
	{
		vector3 position = vector3::zero;
		vector3 scale	 = vector3::one;
		quat	rotation = quat::identity;
	};

	struct entity_version_cache
	{
		u32 local_version			  = 0;
		u32 abs_version				  = 0;
		u32 cached_parent_abs_version = 0;
		u32 cached_local_version	  = 0;
	};

	class entity_manager
	{
	public:
		entity_manager() = delete;
		entity_manager(world& w);
		~entity_manager();

		void init();
		void uninit();
		void calculate_abs_transforms();

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		void interpolate_entities(double interpolation);
		void set_previous_transforms();
#endif

		// -----------------------------------------------------------------------------
		// entity api
		// -----------------------------------------------------------------------------

		world_handle				create_entity(const char* name = "entity");
		world_handle				find_entity(const char* name);
		world_handle				find_entity(world_handle parent, const char* name);
		void						destroy_entity(world_handle handle);
		world_handle				clone_entity(world_handle source, world_handle target_parent = {});
		void						add_child(world_handle parent, world_handle child);
		void						remove_child(world_handle parent, world_handle child);
		void						remove_from_parent(world_handle entity);
		world_handle				get_child_by_index(world_handle parent, u32 index);
		const aabb&					get_entity_aabb(world_handle entity);
		const entity_meta&			get_entity_meta(world_handle entity) const;
		const resource_handle&		get_entity_template_ref(world_handle entity) const;
		const entity_family&		get_entity_family(world_handle entity) const;
		const bitmask<u16>			get_entity_flags(world_handle entity) const;
		void						set_entity_name(world_handle entity, const char* name);
		void						set_entity_tag(world_handle entity, const char* tag);
		world_handle				find_entity_by_tag(const char* tag);
		void						find_entities_by_tag(const char* tag, vector<world_handle>& out) const;
		void						set_entity_transient(world_handle entity, bool is_transient);
		void						add_render_proxy(world_handle entity);
		void						remove_render_proxy(world_handle entity);
		void						set_entity_visible(world_handle entity, bool is_visible);
		void						set_entity_template(world_handle entity, resource_handle template_ref);
		void						remove_all_entity_components(world_handle entity);
		world_handle				get_valid_handle_by_index(world_id id);
		world_handle				get_entity_component(string_id comp_type, world_handle entity);
		const entity_comp_register& get_component_register(world_handle entity) const;

		// -----------------------------------------------------------------------------
		// util
		// -----------------------------------------------------------------------------

		world_handle instantiate_model(resource_handle model_handle);
		world_handle instantiate_template(const entity_template_raw& er);
		world_handle instantiate_template(resource_handle template_handle);

#ifdef SFG_TOOLMODE
		void reload_instantiated_model(resource_handle old, resource_handle new_h);
#endif

		// -----------------------------------------------------------------------------
		// templates
		// -----------------------------------------------------------------------------

		// world_handle spawn_template(resource_handle templ, const vector3& pos, const quat& rot, const vector3& scale);

		inline bool is_valid(world_handle entity) const
		{
			return _entities->is_valid(entity);
		}

		template <typename VisitFunc> void visit_children_deep(world_handle parent, VisitFunc f)
		{
			const entity_family& fam	= get_entity_family(parent);
			world_handle		 entity = fam.first_child;

			while (!entity.is_null())
			{
				f(entity);
				visit_children(entity, f);
				entity = get_entity_family(entity).next_sibling;
			}
		}

		template <typename VisitFunc> void visit_children(world_handle parent, VisitFunc f)
		{
			const entity_family& fam	= get_entity_family(parent);
			world_handle		 entity = fam.first_child;

			while (!entity.is_null())
			{
				f(entity);
				entity = get_entity_family(entity).next_sibling;
			}
		}

		template <typename VisitFunc> void visit_parents(world_handle entity, VisitFunc f)
		{
			const entity_family& fam	= get_entity_family(entity);
			world_handle		 parent = fam.parent;
			while (!parent.is_null())
			{
				f(parent);
				visit_parents(parent, f);
				parent = get_entity_family(parent).parent;
			}
		}

		template <typename T> world_handle get_entity_trait(world_handle entity)
		{
			SFG_ASSERT(is_valid(entity));
			const entity_comp_register& reg = _comp_registers->get(entity.index);
			for (const entity_comp& t : reg.comps)
			{
				if (t.comp_type == type_id<T>::value)
					return t.comp_handle;
			}
			return {};
		}

		template <typename T> world_handle get_entity_component(world_handle entity)
		{
			const string_id id = type_id<T>::value;
			return get_entity_component(id, entity);
		}

		// -----------------------------------------------------------------------------
		// transform api
		// -----------------------------------------------------------------------------

		void			 teleport_entity(world_handle entity);
		void			 set_entity_position(world_handle entity, const vector3& pos);
		void			 set_entity_position_abs(world_handle entity, const vector3& pos);
		void			 set_entity_rotation(world_handle entity, const quat& rot);
		void			 set_entity_rotation_abs(world_handle entity, const quat& rot);
		void			 set_entity_scale(world_handle entity, const vector3& scale);
		void			 set_entity_scale_abs(world_handle entity, const vector3& scale);
		const vector3&	 get_entity_position(world_handle entity) const;
		const quat&		 get_entity_rotation(world_handle entity) const;
		const vector3&	 get_entity_scale(world_handle entity) const;
		vector3			 get_entity_position_abs(world_handle entity, bool recalculate = true);
		const quat&		 get_entity_rotation_abs(world_handle entity, bool recalculate = true);
		vector3			 get_entity_scale_abs(world_handle entity, bool recalculate = true);
		matrix4x3		 get_entity_matrix(world_handle entity) const;
		const matrix4x3& get_entity_matrix_abs(world_handle entity);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline world& get_world()
		{
			return _world;
		}

		inline auto get_entities()
		{
			return _entities;
		}

		inline void set_main_camera(world_handle entity, world_handle comp)
		{
			_camera_entity = entity;
			_camera_comp   = comp;
		}

		inline world_handle get_main_camera_entity() const
		{
			return _camera_entity;
		}

		inline world_handle get_main_camera_comp() const
		{
			return _camera_comp;
		}

#ifdef SFG_TOOLMODE

		inline u8 get_hierarchy_dirty() const
		{
			return _hierarchy_dirty;
		}

		inline void set_hierarchy_dirty(u8 d)
		{
			_hierarchy_dirty = d;
		}

#endif
	private:
		friend class component_manager;

		void calculate_abs_transform(world_id entity);
		void calculate_abs_transform_direct(world_id entity);
		void calculate_abs_rot_direct(world_id entity);
		void calculate_abs_transform_and_rot_direct(world_id entity);

		void on_component_added(world_handle entity, world_handle comp_handle, string_id comp_type);
		void on_component_removed(world_handle entity, world_handle comp_handle, string_id comp_type);
		void reset_all_entity_data();
		void reset_entity_data(world_handle handle);
		void update_entity_flags_to_render(world_handle handle);

	private:
		struct instantiated_model
		{
			world_handle	root = {};
			resource_handle res	 = {};
		};
		world& _world;

		pool_allocator_gen<world_id, world_id, MAX_ENTITIES>* _entities			   = {};
		static_array<resource_handle, MAX_ENTITIES>*		  _template_references = {};
		static_array<entity_meta, MAX_ENTITIES>*			  _metas			   = {};
		static_array<entity_family, MAX_ENTITIES>*			  _families			   = {};
		static_array<aabb, MAX_ENTITIES>*					  _aabbs			   = {};
		static_array<entity_comp_register, MAX_ENTITIES>*	  _comp_registers	   = {};
		static_array<entity_transform, MAX_ENTITIES>*		  _local_transforms	   = {};
		static_array<bitmask<u16>, MAX_ENTITIES>*			  _flags			   = {};
		static_array<matrix4x3, MAX_ENTITIES>*				  _abs_matrices		   = {};
		static_array<quat, MAX_ENTITIES>*					  _abs_rots			   = {};

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		static_array<entity_transform, MAX_ENTITIES>* _prev_local_transforms   = {};
		static_array<entity_transform, MAX_ENTITIES>* _render_local_transforms = {};
#endif

		vector<instantiated_model> _instantiated_models = {};

		static_vector<world_handle, MAX_ENTITIES>* _proxy_entities = {};

		world_handle _camera_entity = {};
		world_handle _camera_comp	= {};

#ifdef SFG_TOOLMODE
		u8 _hierarchy_dirty = 0;
#endif
	};

}
