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

#include "comp_canvas.hpp"
#include "reflection/reflection.hpp"
#include "math/math.hpp"
#include "world/world.hpp"
#include "gui/vekt.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "resources/material.hpp"
#include "resources/font.hpp"

namespace SFG
{
	void comp_canvas::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_canvas>::value, 0, "component");
	}

	void comp_canvas::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);

		chunk_allocator32& aux = w.get_comp_manager().get_aux();
		_builder			   = new vekt::builder();
	}

	void comp_canvas::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);

		if (_flags.is_set(comp_canvas::flags::is_init))
			uninit_builder(w);

		delete _builder;
		_builder = nullptr;
	}

	void comp_canvas::update_counts_and_init(world& w, u32 max_widget_count, u16 max_buffer_count)
	{
		_max_buffer_count = max_buffer_count;
		_max_widget_count = max_widget_count;

		if (_flags.is_set(comp_canvas::flags::is_init))
			uninit_builder(w);
		init_builder(w);
	}

	void comp_canvas::draw(world& w, const vector2ui16& size)
	{
		SFG_ASSERT(_flags.is_set(comp_canvas::flags::is_init));

		_world = &w;

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::canvas_reset_draws,
		});

		_vtx_counter = _idx_counter = 0;

		const vector2 draw_size = vector2ui16(_size.x == 0 ? size.x : _size.x, _size.y == 0 ? size.y : _size.y);
		_builder->build_begin(draw_size);
		_builder->build_end();

		const vekt::vector<vekt::draw_buffer>& draw_buffers = _builder->get_draw_buffers();
		for (const vekt::draw_buffer& db : draw_buffers)
			draw_vekt(db);
	}

	void comp_canvas::set_is_3d(world& w, u8 is_3d)
	{
		_flags.set(comp_canvas::flags::is_3d, is_3d);

		if (_flags.is_set(comp_canvas::flags::is_init))
		{
			const render_event_canvas ev = {
				.entity_index = _header.entity.index,
				.is_3d		  = _flags.is_set(comp_canvas::flags::is_3d),
			};

			w.get_render_stream().add_event(
				{
					.index		= _header.own_handle.index,
					.event_type = render_event_type::canvas_update,
				},
				ev);
		}
	}

	void comp_canvas::init_builder(world& w)
	{
		const vekt::builder::init_config cnf = {
			.widget_count				 = _max_widget_count,
			.vertex_buffer_sz			 = _max_widget_count * 1024,
			.index_buffer_sz			 = _max_widget_count * 1024,
			.text_cache_vertex_buffer_sz = _max_widget_count * 1024,
			.text_cache_index_buffer_sz	 = _max_widget_count * 1024,
			.buffer_count				 = _max_buffer_count,
		};

		_builder->init(cnf);
		_flags.set(comp_canvas::flags::is_init);

		const render_event_canvas ev = {
			.entity_index = _header.entity.index,
			.vertex_size  = static_cast<u32>(cnf.vertex_buffer_sz),
			.index_size	  = static_cast<u32>(cnf.index_buffer_sz),
			.is_3d		  = _flags.is_set(comp_canvas::flags::is_3d),
		};

		w.get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::create_canvas,
			},
			ev);
	}

	void comp_canvas::uninit_builder(world& w)
	{
		_builder->uninit();
		_flags.remove(comp_canvas::flags::is_init);

		w.get_render_stream().add_event({
			.index		= _header.own_handle.index,
			.event_type = render_event_type::destroy_canvas,
		});
	}

	void comp_canvas::draw_vekt(const vekt::draw_buffer& buffer)
	{
		render_event_canvas_add_draw ev = {};

		const vector4 clip = buffer.clip;

		if (clip.y < 0.0f)
		{
			ev.clip.y = 0;
			ev.clip.w = static_cast<u16>(math::max(0.0f, clip.w + clip.y));
		}
		else
		{
			ev.clip.y = static_cast<u16>(clip.y);
			ev.clip.w = static_cast<u16>(clip.w);
		}
		if (clip.x < 0.0f)
		{
			ev.clip.x = 0;
			ev.clip.z = math::min((u16)0, static_cast<u16>(clip.x + clip.z));
		}
		else
		{
			ev.clip.x = static_cast<u16>(clip.x);
			ev.clip.z = static_cast<u16>(clip.z);
		}

		ev.start_index		= _idx_counter;
		ev.start_vertex		= _vtx_counter;
		ev.index_count		= buffer.index_count;
		ev.vertex_data		= reinterpret_cast<u8*>(buffer.vertex_start);
		ev.vertex_data_size = buffer.vertex_count * sizeof(vekt::vertex);
		ev.index_data		= reinterpret_cast<u8*>(buffer.index_start);
		ev.index_data_size	= buffer.index_count * sizeof(vekt::index);

		_idx_counter += buffer.index_count;
		_vtx_counter += buffer.vertex_count;

		void* mat = buffer.user_data;

		resource_handle mat_handle	 = {};
		resource_handle atlas_handle = {};

		if (mat != nullptr)
		{
			mat_handle = *reinterpret_cast<resource_handle*>(mat);
			SFG_ASSERT(!mat || _world->get_resource_manager().is_valid<material>(mat_handle));
		}
		else
		{
			resource_manager& rm = _world->get_resource_manager();

			if (buffer.font_id != NULL_WIDGET_ID)
			{
				mat_handle		= buffer.font_type == vekt::font_type::sdf ? rm.get_default_gui_sdf_mat() : rm.get_default_gui_text_mat();
				atlas_handle	= _world->find_atlas_texture(buffer.font_id);
				ev.atlas_exists = 1;
			}
			else
				mat_handle = rm.get_default_gui_mat();
		}

		ev.material_handle = mat_handle.index;
		ev.atlas_handle	   = atlas_handle.index;

		_world->get_render_stream().add_event(
			{
				.index		= _header.own_handle.index,
				.event_type = render_event_type::canvas_add_draw,
			},
			ev);
	}

}
