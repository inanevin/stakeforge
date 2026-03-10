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

#include "entity_manager.hpp"
#include "world/world.hpp"

// data
#include "data/ostream.hpp"
#include "data/istream.hpp"

// gfx
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_entity.hpp"

// components
#include "world/components/comp_mesh_instance.hpp"
#include "world/components/comp_animation_controller.hpp"
#include "world/components/comp_light.hpp"
#include "world/components/comp_physics.hpp"

// resources
#include "resources/resource_manager.hpp"
#include "resources/entity_template.hpp"
#include "resources/entity_template_utils.hpp"
#include "resources/model.hpp"
#include "resources/light_raw.hpp"
#include "resources/model_node.hpp"
#include "resources/skin.hpp"
#include "resources/common_skin.hpp"
#include "resources/mesh.hpp"
#include "data/hash_map.hpp"

// misc
#include "math/math.hpp"
#include "reflection/reflection.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <tracy/Tracy.hpp>

namespace SFG
{
	entity_manager::entity_manager(world& w) : _world(w)
	{
		_entities			 = new pool_allocator_gen<world_id, world_id, MAX_ENTITIES>();
		_template_references = new static_array<resource_handle, MAX_ENTITIES>();
		_metas				 = new static_array<entity_meta, MAX_ENTITIES>();
		_families			 = new static_array<entity_family, MAX_ENTITIES>();
		_aabbs				 = new static_array<aabb, MAX_ENTITIES>();
		_comp_registers		 = new static_array<entity_comp_register, MAX_ENTITIES>();
		_local_transforms	 = new static_array<entity_transform, MAX_ENTITIES>();
		_flags				 = new static_array<bitmask<u16>, MAX_ENTITIES>();
		_abs_matrices		 = new static_array<matrix4x3, MAX_ENTITIES>();
		_abs_rots			 = new static_array<quat, MAX_ENTITIES>();
		_proxy_entities		 = new static_vector<world_handle, MAX_ENTITIES>();

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		_prev_local_transforms	 = new static_array<entity_transform, MAX_ENTITIES>();
		_render_local_transforms = new static_array<entity_transform, MAX_ENTITIES>();
#endif

#ifdef SFG_TOOLMODE
		_instantiated_models.reserve(512);
#endif
	}

	entity_manager::~entity_manager()
	{
		delete _entities;
		delete _metas;
		delete _template_references;
		delete _families;
		delete _aabbs;
		delete _comp_registers;
		delete _local_transforms;
		delete _flags;
		delete _abs_matrices;
		delete _abs_rots;
		delete _proxy_entities;

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		delete _prev_local_transforms;
		delete _render_local_transforms;
#endif
	}

	void entity_manager::init()
	{
#ifdef SFG_TOOLMODE
		_hierarchy_dirty = 1;
#endif
	}

	void entity_manager::calculate_abs_transform_direct(world_id e)
	{
		const world_handle parent = _families->get(e).parent;
		entity_transform&  local  = _local_transforms->get(e);

		if (parent.is_null())
		{
			matrix4x3& abs = _abs_matrices->get(e);
			abs			   = matrix4x3::transform(local.position, local.rotation, local.scale);
		}
		else
		{
			calculate_abs_transform_direct(parent.index);

			const matrix4x3& parent_abs_mat = _abs_matrices->get(parent.index);
			matrix4x3&		 abs			= _abs_matrices->get(e);
			abs								= parent_abs_mat * matrix4x3::transform(local.position, local.rotation, local.scale);
		}
	}

	void entity_manager::calculate_abs_rot_direct(world_id e)
	{
		const world_handle parent  = _families->get(e).parent;
		quat&			   abs_rot = _abs_rots->get(e);
		entity_transform&  local   = _local_transforms->get(e);

		if (parent.is_null())
		{
			abs_rot = local.rotation;
		}
		else
		{
			calculate_abs_rot_direct(parent.index);
			abs_rot = _abs_rots->get(parent.index) * local.rotation;
		}
	}

	void entity_manager::calculate_abs_transform_and_rot_direct(world_id e)
	{
		const world_handle parent  = _families->get(e).parent;
		quat&			   abs_rot = _abs_rots->get(e);
		entity_transform&  local   = _local_transforms->get(e);

		if (parent.is_null())
		{
			matrix4x3& abs = _abs_matrices->get(e);
			abs			   = matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot		   = local.rotation;
		}
		else
		{
			calculate_abs_transform_and_rot_direct(parent.index);

			const matrix4x3& parent_abs_mat = _abs_matrices->get(parent.index);
			matrix4x3&		 abs			= _abs_matrices->get(e);

			abs		= parent_abs_mat * matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot = _abs_rots->get(parent.index) * local.rotation;
		}
	}

	void entity_manager::calculate_abs_transform(world_id e)
	{
		bitmask<u16>& f = _flags->get(e);

		if (!f.is_set(entity_flags::entity_flags_transient_abs_transform_mark))
			return;

		const world_handle parent = _families->get(e).parent;

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		const entity_transform& local = f.is_set(entity_flags::entity_flags_is_render_proxy) ? _render_local_transforms->get(e) : _local_transforms->get(e);
#else
		const entity_transform& local = _local_transforms->get(e);
#endif

		if (parent.is_null())
		{
			matrix4x3& abs	   = _abs_matrices->get(e);
			quat&	   abs_rot = _abs_rots->get(e);
			abs				   = matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot			   = local.rotation;
		}
		else
		{
			calculate_abs_transform(parent.index);
			matrix4x3& abs	   = _abs_matrices->get(e);
			quat&	   abs_rot = _abs_rots->get(e);
			abs				   = _abs_matrices->get(parent.index) * matrix4x3::transform(local.position, local.rotation, local.scale);
			abs_rot			   = _abs_rots->get(parent.index) * local.rotation;
		}

		f.remove(entity_flags::entity_flags_transient_abs_transform_mark);
	}

	void entity_manager::calculate_abs_transforms()
	{
		ZoneScoped;

		render_event_stream& stream = _world.get_render_stream();

		auto& abs_mats = *_abs_matrices;
		auto& rots	   = *_abs_rots;
		auto& flags	   = *_flags;
		auto& proxies  = *_proxy_entities;

		auto end = _entities->handles_end();

		for (auto it = _entities->handles_begin(); it != end; ++it)
		{
			const auto h = (*it).index;
			flags.get(h).set(entity_flags::entity_flags_transient_abs_transform_mark);
		}

		for (const world_handle& p : proxies)
		{
			const world_id index = p.index;

			bitmask<u16>& ff = flags.get(index);
			if (ff.is_set(entity_flags::entity_flags_invisible))
				continue;

			if (ff.is_set(entity_flags::entity_flags_transient_abs_transform_mark))
				calculate_abs_transform(index);

			matrix4x3& abs_mat = abs_mats.get(index);
			quat&	   abs_rot = rots.get(index);
			stream.add_entity_transform_event(index, abs_mat, abs_rot);
		}
	}

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION

	void entity_manager::set_previous_transforms()
	{
		ZoneScoped;

		render_event_stream& stream = _world.get_render_stream();

		auto& flags = *_flags;

		auto& proxies = *_proxy_entities;

		for (const world_handle& p : proxies)
		{
			const world_id handle_index = p.index;

			bitmask<u16>& ff = flags.get(handle_index);
			if (ff.is_set(entity_flags::entity_flags_invisible))
				continue;

			entity_transform& prev_locals	 = _prev_local_transforms->get(handle_index);
			entity_transform& current_locals = _local_transforms->get(handle_index);
			prev_locals						 = current_locals;
		}
	}

	void entity_manager::interpolate_entities(double interpolation)
	{
		ZoneScoped;

		render_event_stream& stream = _world.get_render_stream();

		const f32 interp = math::clamp((f32)interpolation, 0.0f, 1.0f);

		auto& flags = *_flags;

		auto& proxies = *_proxy_entities;

		for (const world_handle& p : proxies)
		{
			const world_id handle_index = p.index;

			bitmask<u16>& ff = flags.get(handle_index);
			if (ff.is_set(entity_flags::entity_flags_invisible))
				continue;

			entity_transform& prev_locals	 = _prev_local_transforms->get(handle_index);
			entity_transform& current_locals = _local_transforms->get(handle_index);
			entity_transform& render_locals	 = _render_local_transforms->get(handle_index);

			const vector3 pos	   = vector3::lerp(prev_locals.position, current_locals.position, interp);
			const quat	  rot	   = quat::slerp(prev_locals.rotation, current_locals.rotation, interp);
			const vector3 scale	   = vector3::lerp(prev_locals.scale, current_locals.scale, interp);
			render_locals.position = pos;
			render_locals.rotation = rot;
			render_locals.scale	   = scale;
		}
	}

#endif

	void entity_manager::uninit()
	{
		reset_all_entity_data();
	}

	void entity_manager::on_component_added(world_handle entity, world_handle comp_handle, string_id comp_type)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_comp_register& reg = _comp_registers->get(entity.index);

		const entity_comp target = {
			.comp_type	 = comp_type,
			.comp_handle = comp_handle,
		};

		reg.comps.push_back(target);
	}

	void entity_manager::on_component_removed(world_handle entity, world_handle comp_handle, string_id comp_type)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_comp_register& reg = _comp_registers->get(entity.index);

		const entity_comp target = {
			.comp_type	 = comp_type,
			.comp_handle = comp_handle,
		};
		reg.comps.remove_swap(target);
	}

	void entity_manager::reset_all_entity_data()
	{
		_entities->reset();
		_template_references->reset();
		_aabbs->reset();
		_metas->reset();
		_families->reset();
		_comp_registers->reset();
		_local_transforms->reset();
		_flags->reset();
		_abs_matrices->reset();
		_abs_rots->reset();
		_proxy_entities->resize(0);

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		_prev_local_transforms->reset();
		_render_local_transforms->reset();
#endif
	}

	void entity_manager::reset_entity_data(world_handle handle)
	{
		const world_id id	= handle.index;
		entity_meta&   meta = _metas->get(id);
		_world.get_text_allocator().deallocate(meta.name);
		_world.get_text_allocator().deallocate(meta.tag);

		entity_comp_register& reg		   = _comp_registers->get(id);
		auto				  copied_comps = reg.comps;

		component_manager& cm = _world.get_comp_manager();
		for (const entity_comp& t : copied_comps)
			cm.remove_component(t.comp_type, handle, t.comp_handle);

		_template_references->reset(id);
		_aabbs->reset(id);
		_metas->reset(id);
		_families->reset(id);
		_comp_registers->reset(id);
		_local_transforms->reset(id);
		_flags->reset(id);
		_abs_matrices->reset(id);
		_abs_rots->reset(id);

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		_prev_local_transforms->reset(id);
		_render_local_transforms->reset(id);
#endif

		// whatever makes entity proxy is assumed to clean already.
		// _proxy_entities->remove(handle);
	}

	void entity_manager::update_entity_flags_to_render(world_handle handle)
	{
		const bitmask<u16>&				flags = get_entity_flags(handle);
		const render_event_entity_flags ev	  = {
			   .is_visible	= !flags.is_set(entity_flags::entity_flags_invisible),
			   .is_template = flags.is_set(entity_flags::entity_flags_template),
		   };

		_world.get_render_stream().add_event({.index = handle.index, .event_type = render_event_type::update_entity_flags}, ev);
	}

	world_handle entity_manager::create_entity(const char* name)
	{
		world_handle handle = _entities->add();
		set_entity_scale(handle, vector3::one);
		const matrix4x3 def = matrix4x3::transform(vector3::zero, quat::identity, vector3::one);
		//_prev_local_matrices->get(handle.index) = def;
		_abs_matrices->get(handle.index) = def;

		entity_meta& meta = _metas->get(handle.index);
		meta.name		  = _world.get_text_allocator().allocate(name);
		meta.tag		  = _world.get_text_allocator().allocate("");

#ifdef SFG_TOOLMODE
		_hierarchy_dirty = 1;
#endif
		return handle;
	}

	world_handle entity_manager::find_entity(const char* name)
	{
		const auto& entities = *_entities;
		for (auto it = entities.handles_begin(); it != entities.handles_end(); ++it)
		{
			const world_handle handle = *it;
			const char*		   m	  = _metas->get(handle.index).name;
			if (strcmp(m, name) == 0)
				return handle;
		}
		return {};
	}

	world_handle entity_manager::find_entity(world_handle parent, const char* name)
	{
		SFG_ASSERT(_entities->is_valid(parent));

		const entity_family& f = _families->get(parent.index);
		world_handle		 h = {};
		visit_children_deep(parent, [&](world_handle c) {
			const entity_meta& m = get_entity_meta(c);
			if (strcmp(m.name, name) == 0)
			{
				h = c;
				return;
			}
		});

		return h;
	}

	world_handle entity_manager::clone_entity(world_handle source, world_handle target_parent)
	{
		SFG_ASSERT(_entities->is_valid(source));

		hash_map<u32, world_handle> clone_by_source;
		vector<world_handle>		cloned_entities;

		component_manager& cm = _world.get_comp_manager();

		auto clone_recursive = [&](auto&& self, world_handle source_entity, world_handle parent_override) -> world_handle {
			const char*	 src_name = _metas->get(source_entity.index).name;
			world_handle clone	  = create_entity(src_name);
			set_entity_tag(clone, _metas->get(source_entity.index).tag);

			clone_by_source[source_entity.index] = clone;
			cloned_entities.push_back(clone);

			_local_transforms->get(clone.index) = _local_transforms->get(source_entity.index);
#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
			_prev_local_transforms->get(clone.index)   = _prev_local_transforms->get(source_entity.index);
			_render_local_transforms->get(clone.index) = _render_local_transforms->get(source_entity.index);
#endif
			teleport_entity(clone);

			const entity_family& source_family = _families->get(source_entity.index);
			if (parent_override.is_null())
			{
				if (_entities->is_valid(source_family.parent))
					add_child(source_family.parent, clone);
			}
			else
				add_child(parent_override, clone);

			const entity_comp_register& reg = _comp_registers->get(source_entity.index);

			const play_mode pm = _world.get_playmode();
			physics_world&	pw = _world.get_physics_world();

			for (const entity_comp& c : reg.comps)
			{
				void*			   source_ptr = cm.get_component(c.comp_type, c.comp_handle);
				const world_handle new_comp	  = cm.add_component(c.comp_type, clone);
				if (new_comp.is_null())
					continue;
				void* dst_ptr = cm.get_component(c.comp_type, new_comp);

				meta& m = reflection::get().resolve(c.comp_type);

				const bool	skip_reflect_load = c.comp_type == type_id<comp_mesh_instance>::value;
				const auto& fields			  = m.get_fields();
				for (field_base* f : fields)
				{
					void* val  = f->value(source_ptr).get_ptr();
					void* dest = f->value(dst_ptr).get_ptr();

					if (f->_is_list)
					{
						if (f->_type == reflected_field_type::rf_resource)
						{
							vector<resource_handle>& src_v = f->value(source_ptr).cast_ref<vector<resource_handle>>();
							vector<resource_handle>& dst_v = f->value(dst_ptr).cast_ref<vector<resource_handle>>();
							dst_v						   = src_v;
						}
						else if (f->_type == reflected_field_type::rf_entity)
						{
							vector<world_handle>& src_v = f->value(source_ptr).cast_ref<vector<world_handle>>();
							vector<world_handle>& dst_v = f->value(dst_ptr).cast_ref<vector<world_handle>>();
							dst_v						= src_v;
						}
						else
						{
							SFG_ASSERT(false);
						}
					}
					else if (f->_type == reflected_field_type::rf_string)
					{
						string& src_string = f->value(source_ptr).cast_ref<string>();
						string& dst_string = f->value(dst_ptr).cast_ref<string>();
						dst_string		   = src_string;
					}
					else
						SFG_MEMCPY(dest, val, f->get_type_size());

					if (!skip_reflect_load)
						m.invoke_function<void, void*, world&>("on_reflect_load"_hs, dst_ptr, _world);
				}

				if (pm != play_mode::none && c.comp_type == type_id<comp_physics>::value)
				{
					comp_physics& phy = cm.get_component<comp_physics>(c.comp_handle);
					phy.set_is_in_simulation(true);
					pw.add_body_to_world(*phy.create_body(_world));
				}
			}

			const bitmask<u16> fl = _flags->get(source_entity.index);
			set_entity_visible(clone, !fl.is_set(entity_flags::entity_flags_invisible));
			set_entity_template(clone, get_entity_template_ref(source_entity));

			world_handle source_child = source_family.first_child;
			while (!source_child.is_null())
			{
				self(self, source_child, clone);
				source_child = _families->get(source_child.index).next_sibling;
			}

			return clone;
		};

		const world_handle root = clone_recursive(clone_recursive, source, target_parent);

		for (const world_handle& cloned_entity : cloned_entities)
		{
			const entity_comp_register& reg = _comp_registers->get(cloned_entity.index);
			for (const entity_comp& c : reg.comps)
			{
				void* comp_ptr = cm.get_component(c.comp_type, c.comp_handle);
				meta& m		   = reflection::get().resolve(c.comp_type);

				bool updated = false;
				for (field_base* f : m.get_fields())
				{
					if (f->_type != reflected_field_type::rf_entity)
						continue;

					if (f->_is_list)
					{
						vector<world_handle>& v	 = f->value(comp_ptr).cast_ref<vector<world_handle>>();
						const u32			  sz = static_cast<u32>(v.size());
						for (u32 i = 0; i < sz; ++i)
						{
							if (v[i].is_null())
								continue;
							auto it = clone_by_source.find(v[i].index);
							if (it != clone_by_source.end() && v[i] != it->second)
							{
								v[i]	= it->second;
								updated = true;
							}
						}
					}
					else
					{
						world_handle& h = f->value(comp_ptr).cast_ref<world_handle>();
						if (h.is_null())
							continue;
						auto it = clone_by_source.find(h.index);
						if (it != clone_by_source.end() && h != it->second)
						{
							h		= it->second;
							updated = true;
						}
					}
				}

				if ((updated || c.comp_type == type_id<comp_mesh_instance>::value) && m.has_function("on_reflect_load"_hs))
					m.invoke_function<void, void*, world&>("on_reflect_load"_hs, comp_ptr, _world);
			}
		}

		return root;
	}

	void entity_manager::destroy_entity(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_family& fam = _families->get(entity.index);

		// If parent exists, we gotta check for first-child case.
		if (!fam.parent.is_null())
		{
			SFG_ASSERT(_entities->is_valid(fam.parent));
			entity_family& fam_parent = _families->get(fam.parent.index);

			if (fam_parent.first_child == entity)
			{
				// No more first child, and we assign the next sibling if needed.
				fam_parent.first_child = {};

				if (!fam.next_sibling.is_null())
				{
					SFG_ASSERT(_entities->is_valid(fam.next_sibling));
					fam_parent.first_child = fam.next_sibling;
				}
			}
		}

		// Assign next/prev siblings
		if (!fam.prev_sibling.is_null())
		{
			SFG_ASSERT(_entities->is_valid(fam.prev_sibling));
			entity_family& fam_prev = _families->get(fam.prev_sibling.index);
			fam_prev.next_sibling	= fam.next_sibling;
		}

		if (!fam.next_sibling.is_null())
		{
			SFG_ASSERT(_entities->is_valid(fam.next_sibling));
			entity_family& fam_next = _families->get(fam.next_sibling.index);
			fam_next.prev_sibling	= fam.prev_sibling;
		}

		// Destroy children recursively.
		world_handle target_child = fam.first_child;
		while (!target_child.is_null())
		{
			world_handle next = get_entity_family(target_child).next_sibling;
			destroy_entity(target_child);
			target_child = next;
		}

		remove_all_entity_components(entity);
		reset_entity_data(entity);
		_entities->remove(entity);

#ifdef SFG_TOOLMODE
		_hierarchy_dirty = 1;
#endif
	}

	const aabb& entity_manager::get_entity_aabb(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _aabbs->get(entity.index);
	}

	void entity_manager::set_entity_name(world_handle entity, const char* name)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_meta& meta = _metas->get(entity.index);
		_world.get_text_allocator().deallocate(meta.name);
		meta.name = _world.get_text_allocator().allocate(name);
	}

	void entity_manager::set_entity_tag(world_handle entity, const char* tag)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_meta& meta = _metas->get(entity.index);
		_world.get_text_allocator().deallocate(meta.tag);
		meta.tag = _world.get_text_allocator().allocate(tag ? tag : "");
	}

	world_handle entity_manager::find_entity_by_tag(const char* tag)
	{
		const char* target	 = tag ? tag : "";
		const auto& entities = *_entities;
		for (auto it = entities.handles_begin(); it != entities.handles_end(); ++it)
		{
			const world_handle handle = *it;
			const char*		   t	  = _metas->get(handle.index).tag;
			if (strcmp(t, target) == 0)
				return handle;
		}
		return {};
	}

	void entity_manager::find_entities_by_tag(const char* tag, vector<world_handle>& out) const
	{
		out.resize(0);
		const char* target	 = tag ? tag : "";
		const auto& entities = *_entities;
		for (auto it = entities.handles_begin(); it != entities.handles_end(); ++it)
		{
			const world_handle handle = *it;
			const char*		   t	  = _metas->get(handle.index).tag;
			if (strcmp(t, target) == 0)
				out.push_back(handle);
		}
	}

	void entity_manager::set_entity_transient(world_handle entity, bool is_transient)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_flags->get(entity.index).set(entity_flags::entity_flags_no_save, is_transient);
	}

	void entity_manager::add_child(world_handle parent, world_handle child_to_add)
	{
		SFG_ASSERT(_entities->is_valid(parent));
		SFG_ASSERT(_entities->is_valid(child_to_add));
		entity_family& fam_parent = _families->get(parent.index);
		entity_family& fam_child  = _families->get(child_to_add.index);

		// remove from current parent if exists
		if (_entities->is_valid(fam_child.parent))
			remove_child(fam_child.parent, child_to_add);

		fam_child.parent = parent;

		// If first child just add.
		if (fam_parent.first_child.is_null())
		{
			fam_parent.first_child = child_to_add;
			fam_child.prev_sibling = fam_child.next_sibling = {};
		}
		else
		{
			SFG_ASSERT(_entities->is_valid(fam_parent.first_child));
			entity_family& fam_first_child = _families->get(fam_parent.first_child.index);

			// We gotta find the last child. Start with the first one, iterate until next one is null.
			world_handle last_child = fam_parent.first_child;
			world_handle next_child = last_child;
			while (!next_child.is_null())
			{
				SFG_ASSERT(_entities->is_valid(next_child));
				entity_family& fam_current = _families->get(next_child.index);
				last_child				   = next_child;
				next_child				   = fam_current.next_sibling;
			}

			SFG_ASSERT(!last_child.is_null());
			SFG_ASSERT(_entities->is_valid(last_child));
			entity_family& fam_found_last_child = _families->get(last_child.index);
			fam_found_last_child.next_sibling	= child_to_add;
			fam_child.prev_sibling				= last_child;
		}

#ifdef SFG_TOOLMODE
		_hierarchy_dirty = 1;
#endif
	}

	void entity_manager::remove_child(world_handle parent, world_handle child_to_remove)
	{
		SFG_ASSERT(_entities->is_valid(parent));
		SFG_ASSERT(_entities->is_valid(child_to_remove));

		entity_family& fam_parent = _families->get(parent.index);
		entity_family& fam_child  = _families->get(child_to_remove.index);

		if (fam_parent.first_child == child_to_remove)
		{
			// invalidate first child.
			// we assign next sibling as first if applicable.
			fam_parent.first_child = {};

			if (!fam_child.next_sibling.is_null())
			{
				SFG_ASSERT(_entities->is_valid(fam_child.next_sibling));

				fam_parent.first_child = fam_child.next_sibling;

				// also invalidate prev sibling as it was assigned as first.
				entity_family& fam_next_sibling = _families->get(fam_child.next_sibling.index);
				fam_next_sibling.prev_sibling	= {};
			}
		}
		else
		{
			// If not the first, there must be a prev sibling.
			// Find it, give it our current next sibling.
			SFG_ASSERT(!fam_child.prev_sibling.is_null());
			SFG_ASSERT(_entities->is_valid(fam_child.prev_sibling));

			entity_family& fam_prev_sibling = _families->get(fam_child.prev_sibling.index);
			fam_prev_sibling.next_sibling	= fam_child.next_sibling;

			// If our current next sibling is alive, give it our current prev sibling.
			if (!fam_child.next_sibling.is_null())
			{
				SFG_ASSERT(_entities->is_valid(fam_child.next_sibling));
				entity_family& fam_next_sibling = _families->get(fam_child.next_sibling.index);
				fam_next_sibling.prev_sibling	= fam_child.prev_sibling;
			}
		}

		fam_child.next_sibling = {};
		fam_child.prev_sibling = {};
		fam_child.parent	   = {};
	}

	void entity_manager::remove_from_parent(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		const entity_family& family = _families->get(entity.index);
		SFG_ASSERT(_entities->is_valid(family.parent));
		remove_child(family.parent, entity);

#ifdef SFG_TOOLMODE
		_hierarchy_dirty = 1;
#endif
	}

	world_handle entity_manager::get_child_by_index(world_handle entity, u32 index)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		const entity_family& family = _families->get(entity.index);

		if (family.first_child.is_null())
			return {};

		world_handle target = family.first_child;

		for (u32 i = 0; i < index; i++)
		{
			entity_family& fam	= _families->get(target.index);
			world_handle   next = fam.next_sibling;
			if (next.is_null())
				break;
			target = next;
		}

		return target;
	}

	const entity_meta& entity_manager::get_entity_meta(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _metas->get(entity.index);
	}

	const resource_handle& entity_manager::get_entity_template_ref(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _template_references->get(entity.index);
	}

	const entity_family& entity_manager::get_entity_family(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _families->get(entity.index);
	}

	const bitmask<u16> entity_manager::get_entity_flags(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _flags->get(entity.index);
	}

	void entity_manager::add_render_proxy(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_meta& meta = _metas->get(entity.index);

		if (meta.render_proxy_count == 0)
			_proxy_entities->push_back(entity);

		meta.render_proxy_count++;
		_flags->get(entity.index).set(entity_flags::entity_flags_is_render_proxy);

		_world.get_render_stream().add_event({.index = entity.index, .event_type = render_event_type::create_entity});
	}

	void entity_manager::remove_render_proxy(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_meta& meta = _metas->get(entity.index);
		SFG_ASSERT(meta.render_proxy_count > 0);

		meta.render_proxy_count--;
		if (meta.render_proxy_count == 0)
		{
			_proxy_entities->remove(entity);
			_flags->get(entity.index).remove(entity_flags::entity_flags_is_render_proxy);
			_world.get_render_stream().add_event({.index = entity.index, .event_type = render_event_type::remove_entity});
		}
	}

	void entity_manager::set_entity_visible(world_handle entity, bool is_visible)
	{
		SFG_ASSERT(is_valid(entity));
		_flags->get(entity.index).set(entity_flags::entity_flags_invisible, !is_visible);
		update_entity_flags_to_render(entity);
	}

	void entity_manager::set_entity_template(world_handle entity, resource_handle template_ref)
	{
		SFG_ASSERT(is_valid(entity));
		_flags->get(entity.index).set(entity_flags::entity_flags_template, !template_ref.is_null());
		update_entity_flags_to_render(entity);
		_template_references->get(entity.index) = template_ref;
	}

	void entity_manager::remove_all_entity_components(world_handle entity)
	{
		SFG_ASSERT(is_valid(entity));
		entity_comp_register& reg = _comp_registers->get(entity.index);
		component_manager&	  cm  = _world.get_comp_manager();

		for (const entity_comp& c : reg.comps)
		{
			cm.remove_component(c.comp_type, entity, c.comp_handle);
		}

		reg.comps.resize(0);
	}
	/*
	world_handle entity_manager::spawn_template(resource_handle templ, const vector3& pos, const quat& rot, const vector3& scale)
	{
		resource_manager& rm = _world.get_resource_manager();
		SFG_ASSERT(rm.is_valid<entity_template>(templ));
		entity_template&		   t   = rm.get_resource<entity_template>(templ);
		const entity_template_raw& raw = t.get_raw();

		static_vector<world_handle, 1024> created = {};

		component_manager& cm = _world.get_comp_manager();

		u32 i = 0;
		for (const entity_template_entity_raw& r : raw.entities)
		{
			const world_handle h = create_entity(r.name.c_str());

			// add components

			for (const entity_template_component_raw& c : r.components)
			{
			}
			// transformation
			set_entity_position(h, r.position);
			set_entity_rotation(h, r.rotation);
			set_entity_scale(h, r.scale);

			// set vis
			set_entity_visible(h, r.visible);

			created[i] = h;
			i++;
		}

		return {};
	}
	*/
	world_handle entity_manager::get_valid_handle_by_index(world_id id)
	{
		const world_id gen = _entities->get_generation(id);
		return {
			.generation = gen,
			.index		= id,
		};
	}

	world_handle entity_manager::get_entity_component(string_id comp_type, world_handle entity)
	{
		entity_comp_register& reg = _comp_registers->get(entity.index);
		for (const entity_comp& c : reg.comps)
		{
			if (c.comp_type == comp_type)
				return c.comp_handle;
		}

		return {};
	}

	const entity_comp_register& entity_manager::get_component_register(world_handle entity) const
	{
		SFG_ASSERT(is_valid(entity));
		const entity_comp_register& reg = _comp_registers->get(entity.index);
		return reg;
	}

	world_handle entity_manager::instantiate_model(resource_handle model_handle)
	{
		resource_manager&  rm = _world.get_resource_manager();
		component_manager& cm = _world.get_comp_manager();

		const model&	   mdl	   = rm.get_resource<model>(model_handle);
		chunk_allocator32& cm_aux  = cm.get_aux();
		chunk_allocator32& res_aux = rm.get_aux();

		const chunk_handle32 meshes		  = mdl.get_created_meshes();
		const chunk_handle32 nodes		  = mdl.get_created_nodes();
		const u16			 meshes_count = mdl.get_mesh_count();
		const u16			 nodes_count  = mdl.get_node_count();

		if (nodes_count == 0 || meshes_count == 0)
			return {};

		char*		 mdl_name = res_aux.get<char>(mdl.get_name());
		world_handle root	  = create_entity(mdl_name);

		model_node*		 ptr_nodes		   = res_aux.get<model_node>(nodes);
		resource_handle* ptr_meshes_handle = res_aux.get<resource_handle>(meshes);

		// -----------------------------------------------------------------------------
		// entity per node, store roots seperately.
		// -----------------------------------------------------------------------------

		vector<world_handle> created_node_entities;
		vector<world_handle> root_entities;
		for (u16 i = 0; i < nodes_count; i++)
		{
			model_node&		   node = ptr_nodes[i];
			const char*		   name = reinterpret_cast<const char*>(res_aux.get(node.get_name().head));
			const world_handle e	= create_entity(name);
			created_node_entities.push_back(e);

			if (node.get_parent_index() == -1)
				root_entities.push_back(e);

			vector3 out_pos	  = vector3::zero;
			quat	out_rot	  = quat::identity;
			vector3 out_scale = vector3::zero;
			node.get_local_matrix().decompose(out_pos, out_rot, out_scale);
			set_entity_position(created_node_entities[i], out_pos);
			set_entity_rotation(created_node_entities[i], out_rot);
			set_entity_scale(created_node_entities[i], out_scale);
		}

		// -----------------------------------------------------------------------------
		// make sure any bone entities are proxied.
		// -----------------------------------------------------------------------------

		const chunk_handle32   skins	   = mdl.get_created_skins();
		const u16			   skins_count = mdl.get_skin_count();
		const resource_handle* skins_ptr   = skins_count == 0 ? nullptr : res_aux.get<resource_handle>(skins);
		vector<world_handle>   skin_entities;
		for (u16 i = 0; i < skins_count; i++)
		{
			const skin&			 sk			  = rm.get_resource<skin>(skins_ptr[i]);
			const chunk_handle32 joints		  = sk.get_joints();
			const u16			 joints_count = sk.get_joints_count();
			const skin_joint*	 joints_ptr	  = res_aux.get<skin_joint>(joints);
			for (u16 j = 0; j < joints_count; j++)
			{
				const u16 idx = joints_ptr[j].model_node_index;
				skin_entities.push_back(created_node_entities[idx]);
				add_render_proxy(created_node_entities[idx]);
			}
		}

		// -----------------------------------------------------------------------------
		// parent-child relationships
		// -----------------------------------------------------------------------------

		for (u16 i = 0; i < nodes_count; i++)
		{
			model_node& node = ptr_nodes[i];
			if (node.get_parent_index() != -1)
				add_child(created_node_entities[node.get_parent_index()], created_node_entities[i]);
		}

		// -----------------------------------------------------------------------------
		// add components, e.g. lights, mesh instances etc.
		// -----------------------------------------------------------------------------

		const u16  lights_count = mdl.get_light_count();
		light_raw* lights_ptr	= nullptr;
		if (lights_count != 0)
			lights_ptr = res_aux.get<light_raw>(mdl.get_created_lights());

		resource_handle* model_materials = nullptr;
		const u16		 model_mat_count = mdl.get_material_count();
		if (model_mat_count != 0)
		{
			model_materials = res_aux.get<resource_handle>(mdl.get_created_materials());
		}
		vector<resource_handle> mi_materials;

		for (u16 i = 0; i < nodes_count; i++)
		{
			model_node&		   node		   = ptr_nodes[i];
			const world_handle entity	   = created_node_entities[i];
			const i16		   light_index = node.get_light_index();
			if (lights_ptr && light_index != -1)
			{
				SFG_ASSERT(light_index < static_cast<i16>(lights_count));
				light_raw& lr = lights_ptr[light_index];

				if (lr.type == light_raw_type::point)
				{
					const world_handle light_handle = cm.add_component<comp_point_light>(entity);
					comp_point_light&  comp_light	= cm.get_component<comp_point_light>(light_handle);
					comp_light.set_values(_world, lr.base_color, lr.range, lr.intensity);
				}
				else if (lr.type == light_raw_type::spot)
				{
					const world_handle light_handle = cm.add_component<comp_spot_light>(entity);
					comp_spot_light&   comp_light	= cm.get_component<comp_spot_light>(light_handle);
					comp_light.set_values(_world, lr.base_color, lr.range, lr.intensity, lr.inner_cone, lr.outer_cone);
				}
				else if (lr.type == light_raw_type::sun)
				{
					const world_handle light_handle = cm.add_component<comp_dir_light>(entity);
					comp_dir_light&	   comp_light	= cm.get_component<comp_dir_light>(light_handle);
					comp_light.set_values(_world, lr.base_color, lr.intensity);
				}
			}

			if (node.get_mesh_index() == -1)
				continue;

			resource_handle skin_handle = {};
			const i16		skin_index	= node.get_skin_index();

			if (skin_index != -1)
			{
				const resource_handle* model_skins = res_aux.get<resource_handle>(mdl.get_created_skins());
				SFG_ASSERT(mdl.get_skin_count() != 0);
				skin_handle = model_skins[skin_index];
			}

			mi_materials.resize(0);

			const resource_handle& mesh_handle = ptr_meshes_handle[node.get_mesh_index()];
			const mesh&			   m		   = rm.get_resource<mesh>(mesh_handle);

			const u16 mesh_mat_indices_count = m.get_material_count();

			if (mesh_mat_indices_count != 0)
			{
				u16* mesh_mat_indices = res_aux.get<u16>(m.get_material_indices());
				for (u16 i = 0; i < mesh_mat_indices_count; i++)
				{
					const u16 mat_index = mesh_mat_indices[i];
					SFG_ASSERT(mat_index < mdl.get_material_count());
					mi_materials.push_back(model_materials[mat_index]);
				}
			}

			if (!skin_handle.is_null())
			{
				const world_handle		   comp_handle = cm.add_component<comp_animation_controller>(entity);
				comp_animation_controller& ac		   = cm.get_component<comp_animation_controller>(comp_handle);
				ac.set_skin_entities(_world, created_node_entities.data(), static_cast<u16>(created_node_entities.size()));
			}

			const world_handle	comp_handle = cm.add_component<comp_mesh_instance>(entity);
			comp_mesh_instance& mi			= cm.get_component<comp_mesh_instance>(comp_handle);
			mi.set_mesh(_world, mesh_handle, skin_handle, mi_materials.data(), static_cast<u32>(mi_materials.size()), skin_index != -1 ? created_node_entities.data() : nullptr, skin_index != -1 ? static_cast<u32>(created_node_entities.size()) : 0);
		}

		for (world_handle r : root_entities)
		{
			add_child(root, r);
		}

#ifdef SFG_TOOLMODE
		_instantiated_models.push_back({.root = root, .res = model_handle});
#endif
		return root;
	}

	world_handle entity_manager::instantiate_template(const entity_template_raw& raw)
	{
		resource_manager&  rm = _world.get_resource_manager();
		component_manager& cm = _world.get_comp_manager();

		static_vector<world_handle, 1024> created;

		const u32 sz = static_cast<u32>(raw.entities.size());
		created.resize(raw.entities.size());
		for (u32 i = 0; i < sz; i++)
		{
			const entity_template_entity_raw& r = raw.entities[i];

			const string& template_ref	 = r.template_reference;
			world_handle  created_handle = {};
			if (!template_ref.empty())
			{
				const resource_handle tmp_handle = rm.get_resource_handle_by_hash<entity_template>(TO_SID(r.template_reference));
				created_handle					 = instantiate_template(tmp_handle);
			}
			else
				created_handle = create_entity(r.name.c_str());

			if (created_handle.index == 66)
			{
				int a = 5;
			}
			set_entity_tag(created_handle, r.tag.c_str());
			set_entity_position(created_handle, r.position);
			set_entity_rotation(created_handle, r.rotation);
			set_entity_scale(created_handle, r.scale);
			set_entity_visible(created_handle, r.visible);
			created[i] = created_handle;
		}

		for (u32 i = 0; i < sz; i++)
		{
			const entity_template_entity_raw& r = raw.entities[i];
			if (r.parent != -1)
			{
				add_child(created[r.parent], created[i]);
			}
		}

		istream stream(raw.component_buffer.get_raw(), raw.component_buffer.get_size());
		entity_template_utils::fill_components_from_buffer(stream, created, cm, rm, _world);

		// add physicals
		if (_world.get_playmode() != play_mode::none)
		{
			component_manager& cm = _world.get_comp_manager();

			static vector<JPH::BodyID> bodies;
			bodies.resize(0);

			for (world_handle h : created)
			{
				entity_comp_register& reg = _comp_registers->get(h.index);
				for (const entity_comp& c : reg.comps)
				{
					if (c.comp_type != type_id<comp_physics>::value)
						continue;

					comp_physics& phy = cm.get_component<comp_physics>(c.comp_handle);
					phy.set_is_in_simulation(true);
					bodies.push_back(phy.create_body(_world)->GetID());
				}
			}

			if (!bodies.empty())
				_world.get_physics_world().add_bodies_to_world(bodies.data(), static_cast<u32>(bodies.size()));
		}

		const world_handle root = created.empty() ? world_handle() : created[0];
		return root;
	}

	world_handle entity_manager::instantiate_template(resource_handle template_handle)
	{
		resource_manager&  rm = _world.get_resource_manager();
		component_manager& cm = _world.get_comp_manager();

		const entity_template&	   et	= rm.get_resource<entity_template>(template_handle);
		const entity_template_raw& raw	= et.get_raw();
		const world_handle		   root = instantiate_template(raw);

		if (!root.is_null())
			set_entity_template(root, template_handle);
		return root;
	}

#ifdef SFG_TOOLMODE

	void entity_manager::reload_instantiated_model(resource_handle old, resource_handle new_h)
	{
		auto it = std::find_if(_instantiated_models.begin(), _instantiated_models.end(), [old](const instantiated_model& i) -> bool { return old == i.res; });
		if (it == _instantiated_models.end())
			return;

		if (!is_valid(it->root))
			return;

		const vector3&	   p	  = get_entity_position(it->root);
		const quat&		   q	  = get_entity_rotation(it->root);
		const vector3&	   s	  = get_entity_scale(it->root);
		const world_handle parent = get_entity_family(it->root).parent;

		destroy_entity(it->root);

		it->root = instantiate_model(new_h);
		it->res	 = new_h;

		if (!parent.is_null())
			add_child(parent, it->root);

		set_entity_position(it->root, p);
		set_entity_rotation(it->root, q);
		set_entity_scale(it->root, s);
	}
#endif

	/* ----------------                   ---------------- */
	/* ---------------- entity transforms ---------------- */
	/* ----------------                   ---------------- */

	void entity_manager::teleport_entity(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION
		_prev_local_transforms->get(entity.index) = _local_transforms->get(entity.index);
#endif
	}

	void entity_manager::set_entity_position(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).position = pos;
	}

	void entity_manager::set_entity_rotation(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).rotation = rot;
	}

	void entity_manager::set_entity_scale(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		_local_transforms->get(entity.index).scale = scale;
	}

	void entity_manager::set_entity_position_abs(world_handle entity, const vector3& pos)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		entity_family& fam	  = _families->get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_position(entity, pos);
			return;
		}

		calculate_abs_transform_direct(parent.index);
		const matrix4x3& parent_abs		= _abs_matrices->get(parent.index);
		const vector3	 local_position = parent_abs.inverse() * pos;
		set_entity_position(entity, local_position);
	}

	void entity_manager::set_entity_rotation_abs(world_handle entity, const quat& rot)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_family& fam	  = _families->get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_rotation(entity, rot);
			return;
		}

		calculate_abs_rot_direct(parent.index);
		const quat& parent_rot	   = _abs_rots->get(parent.index);
		const quat	local_rotation = parent_rot.conjugate() * rot;
		set_entity_rotation(entity, local_rotation);
	}

	void entity_manager::set_entity_scale_abs(world_handle entity, const vector3& scale)
	{
		SFG_ASSERT(_entities->is_valid(entity));

		entity_family& fam	  = _families->get(entity.index);
		world_handle   parent = fam.parent;

		if (parent.is_null())
		{
			set_entity_scale(entity, scale);
			return;
		}

		calculate_abs_transform_direct(parent.index);

		const matrix4x3& parent_abs			  = _abs_matrices->get(parent.index);
		const matrix4x3	 parent_abs_inv		  = parent_abs.inverse();
		const quat&		 current_abs_rot	  = get_entity_rotation_abs(entity);
		const matrix4x3	 rot_matrix			  = matrix4x3::rotation(current_abs_rot);
		const matrix4x3	 desired_scale_matrix = matrix4x3::scale(scale);
		const matrix4x3	 desired_abs_linear	  = rot_matrix * desired_scale_matrix;
		const matrix4x3	 new_local_linear	  = parent_abs_inv * desired_abs_linear;
		const vector3	 new_local_scale	  = new_local_linear.get_scale();
		set_entity_scale(entity, new_local_scale);
	}

	const vector3& entity_manager::get_entity_position(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _local_transforms->get(entity.index).position;
	}

	const quat& entity_manager::get_entity_rotation(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _local_transforms->get(entity.index).rotation;
	}

	const vector3& entity_manager::get_entity_scale(world_handle entity) const
	{
		SFG_ASSERT(_entities->is_valid(entity));
		return _local_transforms->get(entity.index).scale;
	}

	vector3 entity_manager::get_entity_position_abs(world_handle entity, bool recalculate)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		if (recalculate)
			calculate_abs_transform_direct(entity.index);
		const matrix4x3& abs = _abs_matrices->get(entity.index);
		return abs.get_translation();
	}

	const quat& entity_manager::get_entity_rotation_abs(world_handle entity, bool recalculate)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		if (recalculate)
			calculate_abs_rot_direct(entity.index);
		return _abs_rots->get(entity.index);
	}

	vector3 entity_manager::get_entity_scale_abs(world_handle entity, bool recalculate)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		if (recalculate)
			calculate_abs_transform_direct(entity.index);
		const matrix4x3& abs = _abs_matrices->get(entity.index);
		return abs.get_scale();
	}

	matrix4x3 entity_manager::get_entity_matrix(world_handle entity) const
	{
		const entity_transform& local = _local_transforms->get(entity.index);
		return matrix4x3::transform(local.position, local.rotation, local.scale);
	}

	const matrix4x3& entity_manager::get_entity_matrix_abs(world_handle entity)
	{
		SFG_ASSERT(_entities->is_valid(entity));
		calculate_abs_transform_direct(entity.index);
		return _abs_matrices->get(entity.index);
	}

}
