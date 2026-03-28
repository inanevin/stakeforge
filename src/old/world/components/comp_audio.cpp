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

#include "comp_audio.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "resources/audio.hpp"
#include "io/log.hpp"

#include <vendor/miniaudio/miniaudio.h>

namespace SFG
{
	void comp_audio::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_audio>::value, 0, "component");
		m.set_title("audio");
		m.set_category("audio");

		m.add_field<&comp_audio::_audio_resource, comp_audio>("resource", reflected_field_type::rf_resource, "", type_id<audio>::value);
		m.add_field<&comp_audio::_volume, comp_audio>("volume", reflected_field_type::rf_float, "", 0.0f, 1.0f);
		m.add_field<&comp_audio::_radius_min, comp_audio>("min_radius", reflected_field_type::rf_float, "");
		m.add_field<&comp_audio::_radius_max, comp_audio>("max_radius", reflected_field_type::rf_float, "");
		m.add_field<&comp_audio::_rolloff, comp_audio>("rolloff", reflected_field_type::rf_float, "");
		m.add_field<&comp_audio::_attenuation, comp_audio>("attenuation", reflected_field_type::rf_enum, "")->_enum_list = {"none", "inverse", "linear", "exponential"};
		m.add_field<&comp_audio::_is_looping, comp_audio>("looping", reflected_field_type::rf_bool, "");
		m.add_field<&comp_audio::_play_on_start, comp_audio>("play_on_start", reflected_field_type::rf_bool, "");
		m.add_control_button("test_play", "play this audio once");

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_audio* c = static_cast<comp_audio*>(params.object_ptr);
			if (params.field_title == "resource"_hs)
				c->set_audio(params.w, c->_audio_resource);
			else if (params.field_title == "volume"_hs)
				c->set_volume(params.w, c->_volume);
			else if (params.field_title == "looping"_hs)
				c->set_looping(params.w, c->_is_looping);
			else if (params.field_title == "attenuation"_hs || params.field_title == "rolloff"_hs || params.field_title == "min_radius"_hs || params.field_title == "max_radius"_hs)
				c->set_attenuation_params(params.w, c->_attenuation, c->_radius_min, c->_radius_max, c->_rolloff);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_audio* c = static_cast<comp_audio*>(obj);
			c->set_audio(w, c->_audio_resource);
			c->set_volume(w, c->_volume);
			c->set_looping(w, c->_is_looping);
			c->set_attenuation_params(w, c->_attenuation, c->_radius_min, c->_radius_max, c->_rolloff);
		});

		m.add_function<void, void*, vector<resource_handle_and_type>&>("gather_resources"_hs, [](void* obj, vector<resource_handle_and_type>& h) {
			comp_audio* c = static_cast<comp_audio*>(obj);
			h.push_back({.handle = c->_audio_resource, .type_id = type_id<audio>::value});
		});

		m.add_function<void, const reflected_button_params&>("on_button"_hs, [](const reflected_button_params& params) {
			comp_audio* c = static_cast<comp_audio*>(params.object_ptr);
			if (params.button_id == "test_play"_hs)
				c->toggle_play(params.w);
		});
	}

	void comp_audio::on_add(world& w)
	{
		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		_ma_sound			   = aux.allocate<ma_sound>(1);
	}

	void comp_audio::on_remove(world& w)
	{
		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();

		// uninit if inited
		if (!_audio_resource.is_null())
		{
			ma_sound* snd = aux.get<ma_sound>(_ma_sound);
			ma_sound_uninit(snd);
		}

		// dealloc sound
		aux.free(_ma_sound);
		_ma_sound		= {};
		_audio_resource = {};
	}

	void comp_audio::play(world& w)
	{
		if (_audio_resource.is_null())
			return;

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

		ma_sound_start(snd);
	}

	void comp_audio::toggle_play(world& w)
	{
		if (_audio_resource.is_null())
			return;

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

		if (ma_sound_is_playing(snd))
			ma_sound_stop(snd);
		else
			ma_sound_start(snd);
	}

	void comp_audio::stop(world& w)
	{
		if (_audio_resource.is_null())
			return;
		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);
		ma_sound_stop(snd);
	}

	void comp_audio::reset(world& w)
	{
		if (_audio_resource.is_null())
			return;
		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);
		ma_sound_seek_to_pcm_frame(snd, 0);
	}

	void comp_audio::set_volume(world& w, f32 volume)
	{
		_volume = volume;

		if (_audio_resource.is_null())
			return;

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

		ma_sound_set_volume(snd, volume);
	}

	void comp_audio::set_looping(world& w, u8 looping)
	{
		_is_looping = looping;

		if (_audio_resource.is_null())
			return;

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

		ma_sound_set_looping(snd, _is_looping);
	}

	void comp_audio::set_audio(world& w, resource_handle handle)
	{
		component_manager& cm  = w.get_comp_manager();
		resource_manager&  rm  = w.get_resource_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);

		if (!_audio_resource.is_null())
		{
			ma_sound_uninit(snd);
			_audio_resource = {};
		}

		_audio_resource = handle;
		if (_audio_resource.is_null())
			return;

		audio& aud = rm.get_resource<audio>(_audio_resource);

		ma_engine*		sound_engine = w.get_audio_manager().get_engine();
		const ma_uint32 flags		 = aud.get_flags().is_set(audio::flags::is_streaming) ? MA_SOUND_FLAG_STREAM : 0;
		const ma_result result		 = ma_sound_init_from_data_source(sound_engine, reinterpret_cast<ma_data_source*>(aud.get_decoder(w)), flags, nullptr, snd);
		if (result != MA_SUCCESS)
		{
			SFG_ERR("failed to init ma_sound for audio!");
			_audio_resource = {};
			return;
		}

		set_sound_params(w, snd);
	}

	void comp_audio::set_attenuation_params(world& w, sound_attenuation att, f32 min_radius, f32 max_radius, f32 rolloff)
	{
		_rolloff	 = rolloff;
		_radius_min	 = min_radius;
		_radius_max	 = max_radius;
		_attenuation = att;

		if (_audio_resource.is_null())
			return;

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);
		ma_sound_set_spatialization_enabled(snd, _attenuation != sound_attenuation::none);
		ma_sound_set_min_distance(snd, _radius_min);
		ma_sound_set_max_distance(snd, _radius_max);
		ma_sound_set_attenuation_model(snd, static_cast<ma_attenuation_model>(_attenuation));
		ma_sound_set_rolloff(snd, _rolloff);
	}

	void comp_audio::set_audio_position(world& w, const vector3& p)
	{
		if (_audio_resource.is_null())
			return;

		component_manager& cm  = w.get_comp_manager();
		chunk_allocator32& aux = cm.get_aux();
		ma_sound*		   snd = aux.get<ma_sound>(_ma_sound);
		ma_sound_set_position(snd, p.x, p.y, p.z);
	}

	void comp_audio::set_sound_params(world& w, ma_sound* snd)
	{
		ma_sound_set_volume(snd, _volume);
		ma_sound_set_looping(snd, _is_looping);
		ma_sound_set_spatialization_enabled(snd, _attenuation != sound_attenuation::none);
		ma_sound_set_min_distance(snd, _radius_min);
		ma_sound_set_max_distance(snd, _radius_max);
		ma_sound_set_attenuation_model(snd, static_cast<ma_attenuation_model>(_attenuation));
		ma_sound_set_rolloff(snd, _rolloff);
	}
}
