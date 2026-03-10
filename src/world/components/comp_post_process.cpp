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

#include "comp_post_process.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"

namespace SFG
{
	void comp_post_process::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_post_process>::value, 0, "component");
		m.set_title("post_process");
		m.set_category("fx");

		m.add_field<&comp_post_process::_bloom_strength, comp_post_process>("bloom_strength", reflected_field_type::rf_float, "");
		m.add_field<&comp_post_process::_exposure, comp_post_process>("exposure", reflected_field_type::rf_float, "");
		m.add_field<&comp_post_process::_tonemap_mode, comp_post_process>("tonemap_mode", reflected_field_type::rf_enum, "")->_enum_list = {"aces", "reinhard", "none"};
		m.add_field<&comp_post_process::_saturation, comp_post_process>("saturation", reflected_field_type::rf_float, "");
		m.add_field<&comp_post_process::_wb_temp, comp_post_process>("wb_temp", reflected_field_type::rf_float, "");
		m.add_field<&comp_post_process::_wb_tint, comp_post_process>("wb_tint", reflected_field_type::rf_float, "");
		m.add_field<&comp_post_process::_reinhard_white_point, comp_post_process>("reinhard_white_point", reflected_field_type::rf_float, "");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_post_process* c = static_cast<comp_post_process*>(params.object_ptr);
			c->set_values(params.w, c->_bloom_strength, c->_exposure, c->_tonemap_mode, c->_saturation, c->_wb_temp, c->_wb_tint, c->_reinhard_white_point);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_post_process* c = static_cast<comp_post_process*>(obj);
			c->set_values(w, c->_bloom_strength, c->_exposure, c->_tonemap_mode, c->_saturation, c->_wb_temp, c->_wb_tint, c->_reinhard_white_point);
		});
	}

	void comp_post_process::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		const render_event_post_process ev = {
			.bloom_strength		  = _bloom_strength,
			.exposure			  = _exposure,
			.tonemap_mode		  = static_cast<i32>(_tonemap_mode),
			.saturation			  = _saturation,
			.wb_temp			  = _wb_temp,
			.wb_tint			  = _wb_tint,
			.reinhard_white_point = _reinhard_white_point,
			.entity_index		  = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_post_process,
			},
			ev);
	}

	void comp_post_process::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::remove_post_process,
		});
	}

	void comp_post_process::set_values(world& w, f32 bloom_strength, f32 exposure, tonemap_mode tonemap, f32 saturation, f32 wb_temp, f32 wb_tint, f32 reinhard_white_point)
	{
		_bloom_strength		  = bloom_strength;
		_exposure			  = exposure;
		_tonemap_mode		  = tonemap;
		_saturation			  = saturation;
		_wb_temp			  = wb_temp;
		_wb_tint			  = wb_tint;
		_reinhard_white_point = reinhard_white_point;

		const render_event_post_process ev = {
			.bloom_strength		  = _bloom_strength,
			.exposure			  = _exposure,
			.tonemap_mode		  = static_cast<i32>(_tonemap_mode),
			.saturation			  = _saturation,
			.wb_temp			  = _wb_temp,
			.wb_tint			  = _wb_tint,
			.reinhard_white_point = _reinhard_white_point,
			.entity_index		  = _header.entity.index,
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::update_post_process,
			},
			ev);
	}

}
