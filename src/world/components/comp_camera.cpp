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

#include "comp_camera.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

namespace SFG
{
	void comp_camera::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_camera>::value, 0, "component");
		m.set_title("camera");
		m.set_category("gameplay");

		m.add_field<&comp_camera::_near, comp_camera>("near", reflected_field_type::rf_float, "", 0.001f, 1000.0f);
		m.add_field<&comp_camera::_far, comp_camera>("far", reflected_field_type::rf_float, "", 0.001f, 1000.0f);
		m.add_field<&comp_camera::_fov_degrees, comp_camera>("fov_degrees", reflected_field_type::rf_float, "", 0.0f, 180.0f);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_camera* c = static_cast<comp_camera*>(params.object_ptr);
			c->set_values(params.w, c->_near, c->_far, c->_fov_degrees, c->_cascades);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_camera* c = static_cast<comp_camera*>(obj);
			c->set_values(w, c->_near, c->_far, c->_fov_degrees, c->_cascades);
		});
	}

	void comp_camera::set_values(world& w, f32 near_plane, f32 far_plane, f32 fov_degrees, std::initializer_list<f32> cascades)
	{
		_near		 = near_plane;
		_far		 = far_plane;
		_fov_degrees = fov_degrees;
		_cascades	 = cascades;

		const render_event_camera ev = {
			.cascades	  = _cascades,
			.entity_index = _header.entity.index,
			.near_plane	  = _near,
			.far_plane	  = _far,
			.fov_degrees  = _fov_degrees,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_camera,
			},
			ev);
	}

	void comp_camera::set_values(world& w, f32 near_plane, f32 far_plane, f32 fov_degrees, const static_vector<f32, MAX_SHADOW_CASCADES>& cascades)
	{
		_near		 = near_plane;
		_far		 = far_plane;
		_fov_degrees = fov_degrees;
		_cascades	 = cascades;

		const render_event_camera ev = {
			.cascades	  = _cascades,
			.entity_index = _header.entity.index,
			.near_plane	  = _near,
			.far_plane	  = _far,
			.fov_degrees  = _fov_degrees,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_camera,
			},
			ev);
	}

	void comp_camera::set_main(world& w)
	{
		w.get_entity_manager().set_main_camera(_header.entity, _header.own_handle);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::set_main_camera,
		});
	}

	void comp_camera::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		const render_event_camera ev = {
			.entity_index = _header.entity.index,
			.near_plane	  = _near,
			.far_plane	  = _far,
			.fov_degrees  = _fov_degrees,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_camera,
			},
			ev);
	}

	void comp_camera::on_remove(world& w)
	{
		entity_manager& em = w.get_entity_manager();

		em.remove_render_proxy(_header.entity);

		if (em.get_main_camera_comp() == _header.own_handle)
			em.set_main_camera({}, {});

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_camera,
		});
	}

}
