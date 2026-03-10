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

#include "comp_light.hpp"
#include "reflection/reflection.hpp"
#include "math/math.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "gfx/event_stream/render_event_stream.hpp"

namespace SFG
{

	void comp_dir_light::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_dir_light>::value, 0, "component");
		m.set_title("dir_light");
		m.set_category("light");

		m.add_field<&comp_dir_light::_base_color, comp_dir_light>("color", reflected_field_type::rf_color, "");
		m.add_field<&comp_dir_light::_intensity, comp_dir_light>("intensity", reflected_field_type::rf_float, "");
		m.add_field<&comp_dir_light::_cast_shadows, comp_dir_light>("cast_shadows", reflected_field_type::rf_bool, "");
		m.add_field<&comp_dir_light::_shadow_resolution, comp_dir_light>("shadow_res", reflected_field_type::rf_vector2ui16, "");
		m.add_field<&comp_dir_light::_max_cascades, comp_dir_light>("max_cascades", reflected_field_type::rf_uint8, "", 1.0f, static_cast<f32>(MAX_SHADOW_CASCADES));

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_dir_light* c = static_cast<comp_dir_light*>(params.object_ptr);

			c->_max_cascades = math::clamp(c->_max_cascades, (u8)0, (u8)MAX_SHADOW_CASCADES);
			if (params.field_title == "color"_hs || params.field_title == "intensity"_hs)
				c->set_values(params.w, c->_base_color, c->_intensity);
			else
				c->set_shadow_values(params.w, c->_cast_shadows, c->_max_cascades, c->_shadow_resolution);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_dir_light* c = static_cast<comp_dir_light*>(obj);
			c->set_values(w, c->_base_color, c->_intensity);
			c->set_shadow_values(w, c->_cast_shadows, c->_max_cascades, c->_shadow_resolution);
		});
	}

	void comp_spot_light::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_spot_light>::value, 0, "component");
		m.set_title("spot_light");
		m.set_category("light");

		m.add_field<&comp_spot_light::_base_color, comp_spot_light>("color", reflected_field_type::rf_color, "");
		m.add_field<&comp_spot_light::_range, comp_spot_light>("range", reflected_field_type::rf_float, "");
		m.add_field<&comp_spot_light::_intensity, comp_spot_light>("intensity", reflected_field_type::rf_float, "");
		m.add_field<&comp_spot_light::_inner_cone, comp_spot_light>("inner_cone", reflected_field_type::rf_float, "", 0.0f, 90.0f);
		m.add_field<&comp_spot_light::_outer_cone, comp_spot_light>("outer_cone", reflected_field_type::rf_float, "", 0.0f, 90.0f);
		m.add_field<&comp_spot_light::_cast_shadows, comp_spot_light>("cast_shadows", reflected_field_type::rf_bool, "");
		m.add_field<&comp_spot_light::_near_plane, comp_spot_light>("near_plane", reflected_field_type::rf_float, "", 0.01f, 25.0f);
		m.add_field<&comp_spot_light::_shadow_resolution, comp_spot_light>("shadow_res", reflected_field_type::rf_vector2ui16, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_spot_light* c = static_cast<comp_spot_light*>(params.object_ptr);

			if (params.field_title == "color"_hs || params.field_title == "intensity"_hs || params.field_title == "inner_cone"_hs || params.field_title == "outer_cone"_hs)
				c->set_values(params.w, c->_base_color, c->_range, c->_intensity, c->_inner_cone, c->_outer_cone);
			else
				c->set_shadow_values(params.w, c->_cast_shadows, c->_near_plane, c->_shadow_resolution);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_spot_light* c = static_cast<comp_spot_light*>(obj);
			c->set_values(w, c->_base_color, c->_range, c->_intensity, c->_inner_cone, c->_outer_cone);
			c->set_shadow_values(w, c->_cast_shadows, c->_near_plane, c->_shadow_resolution);
		});
	}

	void comp_point_light::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_point_light>::value, 0, "component");
		m.set_title("point_light");
		m.set_category("light");

		m.add_field<&comp_point_light::_base_color, comp_point_light>("color", reflected_field_type::rf_color, "");
		m.add_field<&comp_point_light::_range, comp_point_light>("range", reflected_field_type::rf_float, "");
		m.add_field<&comp_point_light::_intensity, comp_point_light>("intensity", reflected_field_type::rf_float, "");
		m.add_field<&comp_point_light::_cast_shadows, comp_point_light>("cast_shadows", reflected_field_type::rf_bool, "");
		m.add_field<&comp_point_light::_near_plane, comp_point_light>("near_plane", reflected_field_type::rf_float, "", 0.01f, 25.0f);
		m.add_field<&comp_point_light::_shadow_resolution, comp_point_light>("shadow_res", reflected_field_type::rf_vector2ui16, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_point_light* c = static_cast<comp_point_light*>(params.object_ptr);

			if (params.field_title == "color"_hs || params.field_title == "intensity"_hs || params.field_title == "range"_hs)
				c->set_values(params.w, c->_base_color, c->_range, c->_intensity);
			else
				c->set_shadow_values(params.w, c->_cast_shadows, c->_near_plane, c->_shadow_resolution);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_point_light* c = static_cast<comp_point_light*>(obj);
			c->set_values(w, c->_base_color, c->_range, c->_intensity);
			c->set_shadow_values(w, c->_cast_shadows, c->_near_plane, c->_shadow_resolution);
		});
	}

	void comp_dir_light::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
		send_event(w);
	}

	void comp_dir_light::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_dir_light,
		});
	}

	void comp_dir_light::send_event(world& w)
	{
		render_event_dir_light ev = {};
		ev.entity_index			  = _header.entity.index;
		ev.base_color			  = vector3(_base_color.x, _base_color.y, _base_color.z);
		ev.intensity			  = _intensity;
		ev.cast_shadows			  = _cast_shadows;
		ev.shadow_resolution	  = _shadow_resolution;
		ev.max_cascades			  = 6;

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_dir_light,
			},
			ev);
	}

	void comp_dir_light::set_values(world& w, const color& c, f32 intensity)
	{
		_base_color = c;
		_intensity	= intensity;
		send_event(w);
	}

	void comp_dir_light::set_shadow_values(world& w, u8 cast_shadows, u8 max_cascades, const vector2ui16& resolution)
	{
		_cast_shadows	   = cast_shadows;
		_max_cascades	   = max_cascades;
		_shadow_resolution = resolution;
		send_event(w);
	}

	void comp_spot_light::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
		send_event(w);
	}
	void comp_spot_light::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_spot_light,
		});
	}

	void comp_spot_light::set_values(world& w, const color& c, f32 range, f32 intensity, f32 inner_cone, f32 outer_cone)
	{
		_base_color = c;
		_range		= range;
		_intensity	= intensity;
		_inner_cone = inner_cone;
		_outer_cone = outer_cone;
		send_event(w);
	}

	void comp_spot_light::set_shadow_values(world& w, u8 cast_shadows, f32 near_plane, const vector2ui16& resolution)
	{
		_cast_shadows	   = cast_shadows;
		_shadow_resolution = resolution;
		_near_plane		   = near_plane;
		send_event(w);
	}

	void comp_spot_light::send_event(world& w)
	{
		render_event_spot_light ev = {};
		ev.entity_index			   = _header.entity.index;
		ev.base_color			   = vector3(_base_color.x, _base_color.y, _base_color.z);
		ev.range				   = _range;
		ev.intensity			   = _intensity;
		ev.inner_cone			   = _inner_cone;
		ev.outer_cone			   = _outer_cone;
		ev.cast_shadows			   = _cast_shadows;
		ev.shadow_resolution	   = _shadow_resolution;
		ev.near_plane			   = _near_plane;

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_spot_light,
			},
			ev);
	}

	void comp_point_light::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
		send_event(w);
	}
	void comp_point_light::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_point_light,
		});
	}

	void comp_point_light::set_values(world& w, const color& c, f32 range, f32 intensity)
	{
		_base_color = c;
		_range		= range;
		_intensity	= intensity;
		send_event(w);
	}

	void comp_point_light::set_shadow_values(world& w, u8 cast_shadows, f32 near_plane, const vector2ui16& resolution)
	{
		_cast_shadows	   = cast_shadows;
		_shadow_resolution = resolution;
		_near_plane		   = near_plane;
		send_event(w);
	}

	void comp_point_light::send_event(world& w)
	{
		render_event_point_light ev = {};
		ev.entity_index				= _header.entity.index;
		ev.base_color				= vector3(_base_color.x, _base_color.y, _base_color.z);
		ev.range					= _range;
		ev.intensity				= _intensity;
		ev.cast_shadows				= _cast_shadows;
		ev.shadow_resolution		= _shadow_resolution;
		ev.near_plane				= _near_plane;

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_point_light,
			},
			ev);
	}

}
