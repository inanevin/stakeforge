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

#include "comp_animation_controller.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "resources/res_state_machine.hpp"
#include "resources/animation.hpp"

namespace SFG
{
	void comp_animation_controller::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_animation_controller>::value, 0, "component");
		m.set_title("anim_controller");
		m.set_category("animation");

		m.add_field<&comp_animation_controller::_resource_machine, comp_animation_controller>("machine", reflected_field_type::rf_resource, "", type_id<res_state_machine>::value);
		m.add_field<&comp_animation_controller::_skin_entities, comp_animation_controller>("skin_entities", reflected_field_type::rf_entity, "", (string_id)0, 1, 1);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_animation_controller* c = static_cast<comp_animation_controller*>(params.object_ptr);
			c->set_machine_resource(params.w, c->_resource_machine);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_animation_controller* c = static_cast<comp_animation_controller*>(obj);
			c->set_skin_entities(w, c->_skin_entities.data(), static_cast<u16>(c->_skin_entities.size()));
			c->set_machine_resource(w, c->_resource_machine);
		});

		m.add_function<void, void*, vector<resource_handle_and_type>&>("gather_resources"_hs, [](void* obj, vector<resource_handle_and_type>& h) {
			comp_animation_controller* c = static_cast<comp_animation_controller*>(obj);
			h.push_back({.handle = c->_resource_machine, .type_id = type_id<res_state_machine>::value});
		});
	}

	void comp_animation_controller::on_add(world& w)
	{
	}

	void comp_animation_controller::on_remove(world& w)
	{
		if (!_machine_runtime.is_null())
		{
			animation_graph& anim_graph = w.get_animation_graph();
			anim_graph.remove_state_machine(_machine_runtime);
			_machine_runtime = {};
		}

		if (_skin_entities_ch.size != 0)
		{
			chunk_allocator32& aux = w.get_comp_manager().get_aux();
			aux.free(_skin_entities_ch);
			_skin_entities_ch = {};
		}
	}

	void comp_animation_controller::set_skin_entities(world& w, world_handle* skin_entities, u16 skin_entities_count)
	{
		chunk_allocator32& aux = w.get_comp_manager().get_aux();

		if (_skin_entities_ch.size != 0)
		{
			aux.free(_skin_entities_ch);
			_skin_entities_ch = {};
		}

		_skin_entities.resize(skin_entities_count);
		if (skin_entities_count > 0)
		{
			world_handle* out = nullptr;
			_skin_entities_ch = aux.allocate<world_handle>(skin_entities_count, out);
			for (u16 i = 0; i < skin_entities_count; i++)
			{
				out[i]			  = skin_entities[i];
				_skin_entities[i] = skin_entities[i];
			}
		}

		if (!_machine_runtime.is_null())
		{
			animation_graph&		 anim_graph = w.get_animation_graph();
			animation_state_machine& runtime_sm = anim_graph.get_state_machine(_machine_runtime);
			runtime_sm.joint_entities			= _skin_entities_ch;
			runtime_sm.joint_entities_count		= static_cast<u16>(_skin_entities.size());
		}
	}

	void comp_animation_controller::set_machine_resource(world& w, resource_handle h)
	{
		animation_graph& anim_graph = w.get_animation_graph();

		if (!_machine_runtime.is_null())
		{
			anim_graph.remove_state_machine(_machine_runtime);
			_machine_runtime = {};
		}

		_resource_machine = h;
		if (_resource_machine.is_null())
			return;

		resource_manager&			 rm			= w.get_resource_manager();
		res_state_machine&			 res_sm		= rm.get_resource<res_state_machine>(_resource_machine);
		const res_state_machine_raw& res_sm_raw = res_sm.get_raw();

		_machine_runtime					= anim_graph.create_state_machine_from_raw(w, res_sm_raw);
		animation_state_machine& runtime_sm = anim_graph.get_state_machine(_machine_runtime);
		runtime_sm.entity					= _header.entity;
		runtime_sm.joint_entities			= _skin_entities_ch;
		runtime_sm.joint_entities_count		= static_cast<u16>(_skin_entities.size());
	}

}
