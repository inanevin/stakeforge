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

#include "comp_mesh_instance.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "resources/model.hpp"
#include "resources/mesh.hpp"
#include "resources/skin.hpp"
#include "resources/material.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

namespace SFG
{
	void comp_mesh_instance::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_mesh_instance>::value, 0, "component");
		m.set_title("mesh_instance");
		m.set_category("rendering");

		m.add_field<&comp_mesh_instance::_target_mesh, comp_mesh_instance>("mesh", reflected_field_type::rf_resource, "", type_id<mesh>::value);
		m.add_field<&comp_mesh_instance::_target_skin, comp_mesh_instance>("skin", reflected_field_type::rf_resource, "", type_id<skin>::value);
		m.add_field<&comp_mesh_instance::_materials, comp_mesh_instance>("material", reflected_field_type::rf_resource, "", type_id<material>::value, 1);
		m.add_field<&comp_mesh_instance::_skin_entities, comp_mesh_instance>("skin_entities", reflected_field_type::rf_entity, "", (string_id)0, 1, 1);

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_mesh_instance* c = static_cast<comp_mesh_instance*>(obj);
			c->set_mesh(w, c->_target_mesh, c->_target_skin, c->_materials.data(), static_cast<u16>(c->_materials.size()), c->_skin_entities.data(), static_cast<u16>(c->_skin_entities.size()));

			entity_manager& em = w.get_entity_manager();
			for (world_handle skin_entity : c->_skin_entities)
			{
				if (!em.is_valid(skin_entity))
					continue;

				em.add_render_proxy(skin_entity);
			}
		});

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_mesh_instance* c = static_cast<comp_mesh_instance*>(params.object_ptr);
			if (params.field_title == "material"_hs)
			{
				resource_handle h = *reinterpret_cast<resource_handle*>(params.data_ptr);
				c->set_material(params.w, h, params.list_index);
			}
		});

		m.add_function<void, void*, vector<resource_handle_and_type>&>("gather_resources"_hs, [](void* obj, vector<resource_handle_and_type>& h) {
			comp_mesh_instance* c = static_cast<comp_mesh_instance*>(obj);
			h.push_back({.handle = c->_target_mesh, .type_id = type_id<mesh>::value});
			h.push_back({.handle = c->_target_skin, .type_id = type_id<skin>::value});

			for (const resource_handle handle : c->_materials)
				h.push_back({.handle = handle, .type_id = type_id<material>::value});
		});
	}

	void comp_mesh_instance::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
	}

	void comp_mesh_instance::on_remove(world& w)
	{
		chunk_allocator32& aux = w.get_comp_manager().get_aux();

		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_mesh_instance,
		});
	}

	void comp_mesh_instance::set_mesh(world& w, resource_handle mesh, resource_handle skin, resource_handle* materials, u16 materials_count, world_handle* skin_node_entities, u16 skin_node_entity_count)
	{
		chunk_allocator32& aux = w.get_comp_manager().get_aux();

		_target_mesh = mesh;
		_target_skin = skin;
		_materials.resize(materials_count);
		_skin_entities.resize(skin_node_entity_count);

		for (u32 i = 0; i < materials_count; i++)
			_materials[i] = materials[i];
		for (u16 i = 0; i < skin_node_entity_count; i++)
			_skin_entities[i] = skin_node_entities[i];

		render_event_mesh_instance ev = {};
		ev.entity_index				  = _header.entity.index;
		ev.mesh						  = _target_mesh.index;
		ev.skin						  = skin.is_null() ? NULL_RESOURCE_ID : skin.index;
		for (u16 i = 0; i < materials_count; i++)
			ev.materials.push_back(materials[i].index);

		for (u16 i = 0; i < skin_node_entity_count; i++)
			ev.skin_node_entities.push_back(skin_node_entities[i].index);

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_mesh_instance,
			},
			ev);
	}

	void comp_mesh_instance::set_material(world& w, resource_handle material, u32 index)
	{
		SFG_ASSERT(index < static_cast<u32>(_materials.size()));

		const render_event_mesh_instance_material ev = {
			.material = material.index,
			.index	  = index,
		};
		_materials[index] = material;
		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_mesh_instance_material,
			},
			ev);
	}

	void comp_mesh_instance::fetch_refs(resource_manager& rm, string_id& out_target, string_id& out_target_mesh) const
	{
	}

	void comp_mesh_instance::fill_refs(resource_manager& rm, string_id target_model, string_id target_mesh)
	{
	}

}
