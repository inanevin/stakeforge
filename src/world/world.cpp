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

#include "world.hpp"
#include "game/game_max_defines.hpp"
#include "platform/window.hpp"
#include "app/package_manager.hpp"
#include "gui/vekt.hpp"

// resources
#include "resources/texture.hpp"
#include "resources/texture_raw.hpp"
#include "resources/texture_sampler.hpp"
#include "resources/texture_sampler_raw.hpp"
#include "resources/shader.hpp"
#include "resources/shader_raw.hpp"
#include "resources/material.hpp"
#include "resources/material_raw.hpp"
#include "resources/model.hpp"
#include "resources/model_raw.hpp"
#include "resources/skin.hpp"
#include "resources/skin_raw.hpp"
#include "resources/mesh.hpp"
#include "resources/mesh_raw.hpp"
#include "resources/animation.hpp"
#include "resources/animation_raw.hpp"
#include "resources/res_state_machine.hpp"
#include "resources/res_state_machine_raw.hpp"
#include "resources/audio.hpp"
#include "resources/audio_raw.hpp"
#include "resources/font.hpp"
#include "resources/font_raw.hpp"
#include "resources/physical_material.hpp"
#include "resources/physical_material_raw.hpp"
#include "resources/model_node.hpp"
#include "resources/primitive.hpp"
#include "resources/world_raw.hpp"
#include "resources/particle_properties.hpp"
#include "resources/particle_properties_raw.hpp"
#include "resources/entity_template.hpp"
#include "resources/entity_template_raw.hpp"

// components
#include "components/comp_camera.hpp"
#include "components/comp_light.hpp"
#include "components/comp_mesh_instance.hpp"
#include "components/comp_ambient.hpp"
#include "components/comp_bloom.hpp"
#include "components/comp_ssao.hpp"
#include "components/comp_post_process.hpp"
#include "components/comp_skybox.hpp"
#include "components/comp_physics_settings.hpp"
#include "components/comp_physics.hpp"
#include "components/comp_character_controller.hpp"
#include "components/comp_audio.hpp"
#include "components/comp_canvas.hpp"
#include "components/comp_animation_controller.hpp"
#include "components/comp_particle_emitter.hpp"
#include "components/comp_sprite.hpp"
#include "components/comp_character_controller.hpp"

#include <vendor/miniaudio/miniaudio.h>
#include <tracy/Tracy.hpp>

namespace SFG
{
	world::world(render_event_stream& rstream) : _entity_manager(*this), _comp_manager(*this), _render_stream(rstream), _resource_manager(*this), _phy_world(*this)
	{
		_vekt_atlases.reserve(32);
		_text_allocator.init(MAX_ENTITIES * 32);

		// resource registry
		_resource_manager.register_cache<texture, texture_raw, MAX_WORLD_TEXTURES, 0>();
		_resource_manager.register_cache<texture_sampler, texture_sampler_raw, MAX_WORLD_SAMPLERS, 0>();
		_resource_manager.register_cache<audio, audio_raw, MAX_WORLD_AUDIO, 0>();
		_resource_manager.register_cache<font, font_raw, MAX_WORLD_FONTS, 0>();
		_resource_manager.register_cache<mesh, mesh_raw, MAX_WORLD_MESHES, 0>();
		_resource_manager.register_cache<animation, animation_raw, MAX_WORLD_ANIMS, 0>();
		_resource_manager.register_cache<skin, skin_raw, MAX_WORLD_SKINS, 0>();
		_resource_manager.register_cache<shader, shader_raw, MAX_WORLD_SHADERS, 0>();
		_resource_manager.register_cache<physical_material, physical_material_raw, MAX_WORLD_PHYSICAL_MATERIALS, 0>();

		_resource_manager.register_cache<material, material_raw, MAX_WORLD_MATERIALS, 1>();
		_resource_manager.register_cache<model, model_raw, MAX_WORLD_MODELS, 1>();

		_resource_manager.register_cache<particle_properties, particle_properties_raw, MAX_WORLD_PARTICLE_PROPERTIES, 2>();
		_resource_manager.register_cache<res_state_machine, res_state_machine_raw, MAX_WORLD_ANIM_STATE_MACHINES, 2>();
		_resource_manager.register_cache<entity_template, entity_template_raw, MAX_WORLD_ENTITY_TEMPLATES, 3>();

		// trait registry
		_comp_manager.register_cache<comp_camera, MAX_WORLD_COMP_CAMERAS>();
		_comp_manager.register_cache<comp_point_light, MAX_WORLD_COMP_POINT_LIGHTS>();
		_comp_manager.register_cache<comp_spot_light, MAX_WORLD_COMP_SPOT_LIGHTS>();
		_comp_manager.register_cache<comp_dir_light, MAX_WORLD_COMP_DIR_LIGHTS>();
		_comp_manager.register_cache<comp_mesh_instance, MAX_WORLD_COMP_MESH_INSTANCES>();
		_comp_manager.register_cache<comp_ambient, MAX_WORLD_COMP_AMBIENT>();
		_comp_manager.register_cache<comp_bloom, MAX_WORLD_COMP_BLOOM>();
		_comp_manager.register_cache<comp_ssao, MAX_WORLD_COMP_SSAO>();
		_comp_manager.register_cache<comp_post_process, MAX_WORLD_COMP_POST_PROCESS>();
		_comp_manager.register_cache<comp_skybox, MAX_WORLD_COMP_SKYBOX>();
		_comp_manager.register_cache<comp_physics_settings, MAX_WORLD_COMP_PHYSICS_SETTINGS>();
		_comp_manager.register_cache<comp_physics, MAX_WORLD_COMP_PHYSICS>();
		_comp_manager.register_cache<comp_character_controller, MAX_WORLD_COMP_CHARACTER_CONTROLLERS>();
		_comp_manager.register_cache<comp_audio, MAX_WORLD_COMP_AUDIO>();
		_comp_manager.register_cache<comp_canvas, MAX_WORLD_COMP_CANVAS>();
		_comp_manager.register_cache<comp_animation_controller, MAX_WORLD_COMP_ANIMS>();
		_comp_manager.register_cache<comp_particle_emitter, MAX_WORLD_COMP_PARTICLE_EMITTERS>();
		_comp_manager.register_cache<comp_sprite, MAX_WORLD_COMP_SPRITES>();

		_phy_world.init();
		_audio_manager.init();

		_vekt_fonts = new vekt::font_manager();
		_vekt_fonts->set_callback_user_data(this);
		_vekt_fonts->set_atlas_created_callback(on_atlas_created);
		_vekt_fonts->set_atlas_updated_callback(on_atlas_updated);
		_vekt_fonts->set_atlas_destroyed_callback(on_atlas_destroyed);

		_debug_rendering.init();
	};

	world::~world()
	{
		_audio_manager.uninit();
		_text_allocator.uninit();
		_phy_world.uninit();
		_vekt_fonts->uninit();
		_debug_rendering.uninit();
		delete _vekt_fonts;
	}

	void world::init()
	{
		init_preserve_resources();
		_resource_manager.init();
	}

	void world::uninit()
	{
		uninit_preserve_resources();
		_resource_manager.uninit();
	}

	void world::init_preserve_resources()
	{
		_flags.set(world_flags_is_init);
		_entity_manager.init();
		_comp_manager.init();
		_anim_graph.init();
		_time_manager.init();
	}

	void world::uninit_preserve_resources()
	{
		_comp_manager.uninit();
		_entity_manager.uninit();
		_time_manager.uninit();
		_anim_graph.uninit();
		_text_allocator.reset();
		_flags.remove(world_flags_is_init);
	}

	void world::create_from_loader(world_raw& raw, bool preserve_resources)
	{
		if (preserve_resources)
		{
			uninit_preserve_resources();
			init_preserve_resources();
		}
		else
		{
			uninit();
			init();
		}
		const entity_template_raw& tr = raw.entities_raw;

#ifdef SFG_TOOLMODE
		_tool_camera_pos = raw.tool_cam_pos;
		_tool_camera_rot = raw.tool_cam_rot;
		_resource_manager.load_resources(tr.resources);
#else
		package& pkg = package_manager::get().open_package_res();
		_resource_manager.load_resources(tr.resources, pkg);
		pkg.close();
#endif

		_entity_manager.instantiate_template(tr);
	}

	void world::tick(const vector2ui16& res, f32 dt)
	{
		ZoneScoped;

		// modify dt
		dt = _time_manager.tick(dt);

		const f32 time_speed = _time_manager.get_time_speed();

		_anim_graph.tick(*this, dt);
		_resource_manager.tick();

		if (_play_mode != play_mode::none)
			_phy_world.simulate(dt);

		auto& canvases = _comp_manager.underlying_pool<comp_cache<comp_canvas, MAX_WORLD_COMP_CANVAS>, comp_canvas>();
		for (comp_canvas& c : canvases)
			c.draw(*this, res);

		auto& audios = _comp_manager.underlying_pool<comp_cache<comp_audio, MAX_WORLD_COMP_AUDIO>, comp_audio>();
		for (comp_audio& c : audios)
		{
			if (c.get_attenuation() != sound_attenuation::none)
				c.set_audio_position(*this, _entity_manager.get_entity_position_abs(c.get_header().entity));
		}

		const world_handle mc = _entity_manager.get_main_camera_entity();
		if (!mc.is_null())
		{
			const vector3 p = _entity_manager.get_entity_position_abs(mc);
			ma_engine_listener_set_position(_audio_manager.get_engine(), 0, p.x, p.y, p.z);
		}

		_screen.fetch_camera_data(*this);
	}

	void world::begin_debug_tick(const vector2ui16& res)
	{
		_debug_rendering.begin_frame(res);
	}

	void world::end_debug_tick()
	{
		_debug_rendering.end_frame();
	}

	void world::calculate_abs_transforms()
	{
		_entity_manager.calculate_abs_transforms();
	}

#if FIXED_FRAMERATE_ENABLED && FIXED_FRAMERATE_USE_INTERPOLATION

	void world::interpolate(double interpolation)
	{
		_entity_manager.interpolate_entities(interpolation);
	}

	void world::set_prev_transforms()
	{
		_entity_manager.set_previous_transforms();
	}

#endif

	bool world::on_window_event(const window_event& ev, window* wnd)
	{
		if (!_flags.is_set(world_flags_is_init))
			return false;

		bool handled = false;

		_comp_manager.view<comp_canvas>([&](comp_canvas& cnv) -> comp_view_result {
			vekt::builder* builder = cnv.get_builder();

			const world_handle entity_handle = cnv.get_header().entity;
			const bool		   is_invisible	 = _entity_manager.get_entity_flags(entity_handle).is_set(entity_flags::entity_flags_invisible);
			if (is_invisible)
				return comp_view_result::cont;

			const vekt::input_event_type ev_type = ev.sub_type == window_event_sub_type::press ? vekt::input_event_type::pressed : vekt::input_event_type::released;

			if (ev.type == window_event_type::key)
			{
				const vekt::input_event_result res = builder->on_key_event({
					.type	   = ev_type,
					.key	   = ev.button,
					.scan_code = ev.value.x,
				});

				handled = res == vekt::input_event_result::handled;
			}
			else if (ev.type == window_event_type::mouse)
			{
				const vekt::input_event_result res = builder->on_mouse_event({
					.type	  = ev_type,
					.button	  = ev.button,
					.position = vector2(ev.value.x, ev.value.y),
				});

				handled = res == vekt::input_event_result::handled;
			}
			else if (ev.type == window_event_type::wheel)
			{
				const vekt::input_event_result res = builder->on_mouse_wheel_event({
					.amount = static_cast<f32>(ev.value.x),
				});

				handled = res == vekt::input_event_result::handled;
			}
			else if (ev.type == window_event_type::delta)
			{
				const vector2i16& mp = wnd->get_mouse_position();
				builder->on_mouse_move(vector2(mp.x, mp.y));
			}

			return handled ? comp_view_result::stop : comp_view_result::cont;
		});

		return handled;
	}

	void world::set_playmode(play_mode mode)
	{
		if (mode == _play_mode)
			return;

		if (mode == play_mode::none && _play_mode == play_mode::physics_only)
		{
			_phy_world.uninit_simulation();
		}
		else if (mode == play_mode::none && _play_mode == play_mode::full)
		{
			_phy_world.uninit_simulation();
		}
		else if (mode == play_mode::physics_only && _play_mode == play_mode::full)
		{
			SFG_ERR("Can't switch to physics playmode from full mode.");
			return;
		}
		else if (mode == play_mode::full && _play_mode == play_mode::physics_only)
		{
			SFG_ERR("Can't switch to full playmode from physics only mode.");
			return;
		}
		else if (mode == play_mode::physics_only && _play_mode == play_mode::none)
		{
			_phy_world.init_simulation();
		}
		else if (mode == play_mode::full && _play_mode == play_mode::none)
		{
			_phy_world.init_simulation();
		}

		if (mode == play_mode::full)
		{
			_comp_manager.view<comp_audio>([&](comp_audio& ad) -> comp_view_result {
				if (ad.is_play_on_start())
				{
					ad.reset(*this);
					ad.play(*this);
				}
				return comp_view_result::cont;
			});
		}
		else if (mode == play_mode::none && _play_mode == play_mode::full)
		{
			_comp_manager.view<comp_audio>([&](comp_audio& ad) -> comp_view_result {
				ad.stop(*this);
				ad.reset(*this);
				return comp_view_result::cont;
			});
		}

		_play_mode = mode;
	}

	resource_handle world::find_atlas_texture(vekt::id atlas)
	{
		auto it = std::find_if(_vekt_atlases.begin(), _vekt_atlases.end(), [atlas](const atlas_data& d) -> bool { return d.atlas->get_id() == atlas; });
		SFG_ASSERT(it != _vekt_atlases.end());
		return it->handle;
	}

	void world::on_atlas_created(vekt::atlas* atlas, void* user_data)
	{
		world*			w			 = static_cast<world*>(user_data);
		resource_handle atlas_handle = w->_resource_manager.add_resource<texture>(0);
		texture&		t			 = w->_resource_manager.get_resource<texture>(atlas_handle);

		w->_vekt_atlases.push_back({
			.atlas	= atlas,
			.handle = atlas_handle,
		});
	}

	void world::on_atlas_updated(vekt::atlas* atlas, void* user_data)
	{
		world* w  = static_cast<world*>(user_data);
		auto   it = std::find_if(w->_vekt_atlases.begin(), w->_vekt_atlases.end(), [atlas](const atlas_data& d) -> bool { return d.atlas == atlas; });
		SFG_ASSERT(it != w->_vekt_atlases.end());
		const resource_handle handle = it->handle;

		texture& t = w->_resource_manager.get_resource<texture>(handle);
		t.destroy(*w, handle);

		texture_raw	 raw = {};
		const format fmt = format::r8_unorm;

		const size_t sz = atlas->get_width() * atlas->get_height();

		u8* data = reinterpret_cast<u8*>(SFG_MALLOC(sz));

		if (data != 0)
			SFG_MEMCPY(data, atlas->get_data(), sz);

		raw.load_from_data(data, vector2ui16(atlas->get_width(), atlas->get_height()), static_cast<u8>(fmt), false);
		t.create_from_loader(raw, *w, handle);
	}

	void world::on_atlas_destroyed(vekt::atlas* atlas, void* user_data)
	{
		world* w  = static_cast<world*>(user_data);
		auto   it = std::find_if(w->_vekt_atlases.begin(), w->_vekt_atlases.end(), [atlas](const atlas_data& d) -> bool { return d.atlas == atlas; });
		SFG_ASSERT(it != w->_vekt_atlases.end());

		const resource_handle handle = it->handle;
		if (w->_resource_manager.is_valid<texture>(handle))
		{
			texture& t = w->_resource_manager.get_resource<texture>(handle);
			t.destroy(*w, handle);
			w->_resource_manager.remove_resource<texture>(handle);
		}
		w->_vekt_atlases.erase(it);
	}
}
