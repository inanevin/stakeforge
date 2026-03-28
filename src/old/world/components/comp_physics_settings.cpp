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

#include "comp_physics_settings.hpp"
#include "reflection/type_reflection.hpp"
#include "world/world.hpp"
#include "reflection/reflection.hpp"

namespace SFG
{
	void comp_physics_settings::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_physics_settings>::value, 0, "component");
		m.set_title("physics_settings");
		m.set_category("physics");

		m.add_field<&comp_physics_settings::_gravity, comp_physics_settings>("gravity", reflected_field_type::rf_vector3, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_physics_settings* c = static_cast<comp_physics_settings*>(params.object_ptr);
			c->set_gravity(params.w, c->_gravity);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_physics_settings* c = static_cast<comp_physics_settings*>(obj);
			c->set_gravity(w, c->_gravity);
		});
	}

	void comp_physics_settings::on_add(world& w)
	{
		set_gravity(w, _gravity);
	}

	void comp_physics_settings::on_remove(world& w)
	{
	}

	void comp_physics_settings::set_gravity(world& w, const vector3& gravity)
	{
		_gravity = gravity;
		w.get_physics_world().set_gravity(_gravity);
	}

}
