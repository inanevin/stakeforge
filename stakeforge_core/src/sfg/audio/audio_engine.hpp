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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/common/type_id.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg
{
	class resource_file_system_t;
	struct audio_runtime_t;

	enum class audio_bus_e : u8
	{
		sfx,
		music,
		voice,
		ui,
		count,
	};

	enum class audio_attenuation_e : u8
	{
		none,
		inverse,
		linear,
		exponential,
	};

	SFG_DEFINE_TYPE_ID(audio_bus_e);
	SFG_DEFINE_TYPE_ID(audio_attenuation_e);

	struct audio_voice_tag_t;

	using audio_voice_handle_t = pool_handle_t<u32, audio_voice_tag_t>;

	struct audio_engine_config_t
	{
		u32	 voice_max_count		  = 256;
		u32	 streamed_voice_max_count = 16;
		u32	 stream_worker_count	  = 1;
		bool no_device				  = false;
	};

	struct audio_voice_settings_t
	{
		vec3f_t				position	   = vec3f_t::zero;
		vec3f_t				direction	   = {0.0f, 0.0f, -1.0f};
		vec3f_t				velocity	   = vec3f_t::zero;
		f32					volume		   = 1.0f;
		f32					pitch		   = 1.0f;
		f32					min_distance   = 1.0f;
		f32					max_distance   = 100.0f;
		f32					rolloff		   = 1.0f;
		f32					doppler_factor = 1.0f;
		audio_attenuation_e attenuation	   = audio_attenuation_e::inverse;
		audio_bus_e			bus			   = audio_bus_e::sfx;
		bool				spatialized	   = true;
		bool				looping		   = false;
	};

	struct audio_voice_create_desc_t
	{
		audio_voice_settings_t settings		= {};
		resource_handle_t	   resource		= NULL_RESOURCE_HANDLE;
		bool				   preview		= false;
		bool				   auto_destroy = false;
	};

	class audio_engine_t final
	{
	public:
		audio_engine_t()								 = default;
		~audio_engine_t()								 = default;
		audio_engine_t(const audio_engine_t&)			 = delete;
		audio_engine_t& operator=(const audio_engine_t&) = delete;

		static audio_engine_t& get();

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(resource_file_system_t& resource_file_system, const audio_engine_config_t& config = {});
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// voices
		// -----------------------------------------------------------------------------

		audio_voice_handle_t create_voice(const audio_runtime_t& audio, const audio_voice_create_desc_t& desc);
		void				 destroy_voice(audio_voice_handle_t handle);
		void				 destroy_voices_for_resource(resource_handle_t resource);
		bool				 start_voice(audio_voice_handle_t handle);
		bool				 pause_voice(audio_voice_handle_t handle);
		bool				 stop_voice(audio_voice_handle_t handle);
		bool				 seek_voice(audio_voice_handle_t handle, f32 seconds);
		void				 set_voice_settings(audio_voice_handle_t handle, const audio_voice_settings_t& settings);

		// -----------------------------------------------------------------------------
		// buses
		// -----------------------------------------------------------------------------

		void set_bus_volume(audio_bus_e bus, f32 volume);

		// -----------------------------------------------------------------------------
		// listener
		// -----------------------------------------------------------------------------

		void set_listener(const vec3f_t& position, const vec3f_t& direction, const vec3f_t& velocity);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool is_voice_valid(audio_voice_handle_t handle) const;
		bool is_voice_playing(audio_voice_handle_t handle) const;
		bool is_voice_at_end(audio_voice_handle_t handle) const;
		f32	 get_voice_cursor_seconds(audio_voice_handle_t handle) const;
		f32	 get_bus_volume(audio_bus_e bus) const;
		bool is_init() const;

	private:
		void* _internals = nullptr;
	};
}
