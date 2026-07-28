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

#include "audio_engine.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/gen_pool.hpp>
#include <sfg/runtime/resources/audio.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>
#include <sfg/vendor/miniaudio/miniaudio.h>

namespace sfg
{
	namespace
	{
		struct miniaudio_vfs_t
		{
			ma_vfs_callbacks		callbacks			 = {};
			resource_file_system_t* resource_file_system = nullptr;
		};

		struct audio_voice_t
		{
			ma_resource_manager_data_source stream_data		  = {};
			ma_audio_buffer					resident_data	  = {};
			ma_sound						sound			  = {};
			resource_handle_t				resource		  = NULL_RESOURCE_HANDLE;
			bool							data_initialized  = false;
			bool							sound_initialized = false;
			bool							streamed		  = false;
			bool							auto_destroy	  = false;
		};

		struct audio_engine_internals_t
		{
			ma_resource_manager								  resource_manager							  = {};
			ma_engine										  engine									  = {};
			ma_sound_group									  buses[static_cast<u32>(audio_bus_e::count)] = {};
			ma_sound_group									  preview_bus								  = {};
			gen_pool_t<audio_voice_t, u32, audio_voice_tag_t> voices									  = {};
			miniaudio_vfs_t									  vfs										  = {};
			u32												  streamed_voice_count						  = 0;
			u32												  streamed_voice_max_count					  = 0;
			u32												  initialized_bus_count						  = 0;
			bool											  resource_manager_initialized				  = false;
			bool											  engine_initialized						  = false;
			bool											  preview_bus_initialized					  = false;
		};

		ma_attenuation_model to_miniaudio_attenuation(audio_attenuation_e attenuation)
		{
			switch (attenuation)
			{
			case audio_attenuation_e::none:
				return ma_attenuation_model_none;
			case audio_attenuation_e::linear:
				return ma_attenuation_model_linear;
			case audio_attenuation_e::exponential:
				return ma_attenuation_model_exponential;
			case audio_attenuation_e::inverse:
			default:
				return ma_attenuation_model_inverse;
			}
		}

		ma_result vfs_open(ma_vfs* vfs, const char* path, ma_uint32 open_mode, ma_vfs_file* out_file)
		{
			if (vfs == nullptr || path == nullptr || out_file == nullptr || (open_mode & MA_OPEN_MODE_READ) == 0)
				return MA_INVALID_ARGS;

			u64 resource = 0;
			u64 offset	 = 0;
			u64 size	 = 0;

			if (std::sscanf(path, "sfg_audio:%llu:%llu:%llu", &resource, &offset, &size) != 3)
				return MA_INVALID_FILE;

			miniaudio_vfs_t&   state  = *reinterpret_cast<miniaudio_vfs_t*>(vfs);
			resource_stream_t* stream = new resource_stream_t();

			if (!state.resource_file_system->open_resource_stream(resource, static_cast<size_t>(offset), static_cast<size_t>(size), *stream))
			{
				delete stream;
				return MA_DOES_NOT_EXIST;
			}

			*out_file = stream;
			return MA_SUCCESS;
		}

		ma_result vfs_open_w(ma_vfs* vfs, const wchar_t* path, ma_uint32 open_mode, ma_vfs_file* out_file)
		{
			return MA_NOT_IMPLEMENTED;
		}

		ma_result vfs_close(ma_vfs* vfs, ma_vfs_file file)
		{
			if (vfs == nullptr || file == nullptr)
				return MA_INVALID_ARGS;

			delete static_cast<resource_stream_t*>(file);
			return MA_SUCCESS;
		}

		ma_result vfs_read(ma_vfs* vfs, ma_vfs_file file, void* destination, size_t size, size_t* out_read)
		{
			if (vfs == nullptr || file == nullptr || destination == nullptr)
				return MA_INVALID_ARGS;

			size_t read = 0;

			if (!static_cast<resource_stream_t*>(file)->read(destination, size, read))
				return MA_IO_ERROR;

			if (out_read != nullptr)
				*out_read = read;

			return read == 0 && size != 0 ? MA_AT_END : MA_SUCCESS;
		}

		ma_result vfs_write(ma_vfs* vfs, ma_vfs_file file, const void* source, size_t size, size_t* out_written)
		{
			return MA_NOT_IMPLEMENTED;
		}

		ma_result vfs_seek(ma_vfs* vfs, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
		{
			if (vfs == nullptr || file == nullptr)
				return MA_INVALID_ARGS;

			resource_seek_origin_e resource_origin = resource_seek_origin_e::start;

			switch (origin)
			{
			case ma_seek_origin_current:
				resource_origin = resource_seek_origin_e::current;
				break;
			case ma_seek_origin_end:
				resource_origin = resource_seek_origin_e::end;
				break;
			case ma_seek_origin_start:
			default:
				resource_origin = resource_seek_origin_e::start;
				break;
			}

			return static_cast<resource_stream_t*>(file)->seek(offset, resource_origin) ? MA_SUCCESS : MA_BAD_SEEK;
		}

		ma_result vfs_tell(ma_vfs* vfs, ma_vfs_file file, ma_int64* out_cursor)
		{
			if (vfs == nullptr || file == nullptr || out_cursor == nullptr)
				return MA_INVALID_ARGS;

			*out_cursor = static_cast<ma_int64>(static_cast<resource_stream_t*>(file)->get_cursor());
			return MA_SUCCESS;
		}

		ma_result vfs_info(ma_vfs* vfs, ma_vfs_file file, ma_file_info* out_info)
		{
			if (vfs == nullptr || file == nullptr || out_info == nullptr)
				return MA_INVALID_ARGS;

			*out_info			  = {};
			out_info->sizeInBytes = static_cast<ma_uint64>(static_cast<resource_stream_t*>(file)->get_size());
			return MA_SUCCESS;
		}

		void uninit_voice(audio_engine_internals_t& internals, audio_voice_t& voice)
		{
			if (voice.sound_initialized)
			{
				ma_sound_uninit(&voice.sound);
				voice.sound_initialized = false;
			}

			if (voice.data_initialized)
			{
				if (voice.streamed)
					ma_resource_manager_data_source_uninit(&voice.stream_data);
				else
					ma_audio_buffer_uninit(&voice.resident_data);

				voice.data_initialized = false;
			}

			if (voice.streamed)
			{
				SFG_ASSERT(internals.streamed_voice_count != 0);
				internals.streamed_voice_count--;
			}
		}

		ma_sound_group* get_voice_group(audio_engine_internals_t& internals, const audio_voice_create_desc_t& desc)
		{
			return desc.preview ? &internals.preview_bus : &internals.buses[static_cast<u32>(desc.settings.bus)];
		}

		void apply_settings(audio_voice_t& voice, const audio_voice_settings_t& settings)
		{
			ma_sound_set_volume(&voice.sound, settings.volume);
			ma_sound_set_pitch(&voice.sound, settings.pitch);
			ma_sound_set_spatialization_enabled(&voice.sound, settings.spatialized ? MA_TRUE : MA_FALSE);
			ma_sound_set_position(&voice.sound, settings.position.x, settings.position.y, settings.position.z);
			ma_sound_set_direction(&voice.sound, settings.direction.x, settings.direction.y, settings.direction.z);
			ma_sound_set_velocity(&voice.sound, settings.velocity.x, settings.velocity.y, settings.velocity.z);
			ma_sound_set_attenuation_model(&voice.sound, to_miniaudio_attenuation(settings.attenuation));
			ma_sound_set_min_distance(&voice.sound, settings.min_distance);
			ma_sound_set_max_distance(&voice.sound, settings.max_distance);
			ma_sound_set_rolloff(&voice.sound, settings.rolloff);
			ma_sound_set_doppler_factor(&voice.sound, settings.doppler_factor);
			ma_sound_set_looping(&voice.sound, settings.looping ? MA_TRUE : MA_FALSE);
		}
	}

	audio_engine_t& audio_engine_t::get()
	{
		static audio_engine_t instance;
		return instance;
	}

	bool audio_engine_t::init(resource_file_system_t& resource_file_system, const audio_engine_config_t& config)
	{
		SFG_ASSERT(_internals == nullptr);
		SFG_ASSERT(config.voice_max_count != 0);
		SFG_ASSERT(config.streamed_voice_max_count <= config.voice_max_count);

		audio_engine_internals_t* internals = new audio_engine_internals_t();
		internals->vfs.resource_file_system = &resource_file_system;
		internals->vfs.callbacks.onOpen		= vfs_open;
		internals->vfs.callbacks.onOpenW	= vfs_open_w;
		internals->vfs.callbacks.onClose	= vfs_close;
		internals->vfs.callbacks.onRead		= vfs_read;
		internals->vfs.callbacks.onWrite	= vfs_write;
		internals->vfs.callbacks.onSeek		= vfs_seek;
		internals->vfs.callbacks.onTell		= vfs_tell;
		internals->vfs.callbacks.onInfo		= vfs_info;

		ma_resource_manager_config resource_config = ma_resource_manager_config_init();
		resource_config.decodedFormat			   = ma_format_s16;
		resource_config.decodedSampleRate		   = audio_loader_t::RUNTIME_SAMPLE_RATE;
		resource_config.jobThreadCount			   = config.stream_worker_count;
		resource_config.pVFS					   = reinterpret_cast<ma_vfs*>(&internals->vfs);

		const ma_result resource_result = ma_resource_manager_init(&resource_config, &internals->resource_manager);

		if (resource_result != MA_SUCCESS)
		{
			SFG_ERR("failed to initialize miniaudio resource manager: {0}", static_cast<i32>(resource_result));
			delete internals;
			return false;
		}

		internals->resource_manager_initialized = true;

		ma_engine_config engine_config = ma_engine_config_init();
		engine_config.pResourceManager = &internals->resource_manager;
		engine_config.sampleRate	   = audio_loader_t::RUNTIME_SAMPLE_RATE;
		engine_config.noDevice		   = config.no_device ? MA_TRUE : MA_FALSE;

		const ma_result engine_result = ma_engine_init(&engine_config, &internals->engine);
		if (engine_result != MA_SUCCESS)
		{
			SFG_ERR("failed to initialize miniaudio engine: {0}", static_cast<i32>(engine_result));
			ma_resource_manager_uninit(&internals->resource_manager);
			delete internals;
			return false;
		}

		internals->engine_initialized = true;

		for (u32 i = 0; i < static_cast<u32>(audio_bus_e::count); ++i)
		{
			const ma_result group_result = ma_sound_group_init(&internals->engine, 0, nullptr, &internals->buses[i]);

			if (group_result != MA_SUCCESS)
			{
				SFG_ERR("failed to initialize miniaudio bus: {0}", static_cast<i32>(group_result));
				_internals = internals;
				uninit();
				return false;
			}

			internals->initialized_bus_count++;
		}

		const ma_result preview_result = ma_sound_group_init(&internals->engine, 0, nullptr, &internals->preview_bus);

		if (preview_result != MA_SUCCESS)
		{
			SFG_ERR("failed to initialize miniaudio preview bus: {0}", static_cast<i32>(preview_result));
			_internals = internals;
			uninit();
			return false;
		}

		const ma_device* device = ma_engine_get_device(&internals->engine);

		if (device)
		{
			SFG_INFO("initialized with device: {0}", device->playback.name);
		}

		internals->preview_bus_initialized	= true;
		internals->streamed_voice_max_count = config.streamed_voice_max_count;
		internals->voices.init(config.voice_max_count);
		_internals = internals;

		return true;
	}

	void audio_engine_t::uninit()
	{
		if (_internals == nullptr)
			return;

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);

		for (auto it = internals.voices.begin_handle(); it != internals.voices.end_handle(); ++it)
			uninit_voice(internals, internals.voices.get(*it));

		internals.voices.uninit();

		if (internals.preview_bus_initialized)
			ma_sound_group_uninit(&internals.preview_bus);

		for (u32 i = 0; i < internals.initialized_bus_count; ++i)
			ma_sound_group_uninit(&internals.buses[i]);

		if (internals.engine_initialized)
			ma_engine_uninit(&internals.engine);

		if (internals.resource_manager_initialized)
			ma_resource_manager_uninit(&internals.resource_manager);

		delete &internals;
		_internals = nullptr;
	}

	void audio_engine_t::tick()
	{
		SFG_ASSERT(_internals != nullptr);

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);

		for (auto it = internals.voices.begin_handle(); it != internals.voices.end_handle();)
		{
			const audio_voice_handle_t handle = *it;
			++it;

			audio_voice_t& voice = internals.voices.get(handle);

			if (!voice.auto_destroy || ma_sound_at_end(&voice.sound) == MA_FALSE)
				continue;

			uninit_voice(internals, voice);
			internals.voices.remove(handle);
		}
	}

	audio_voice_handle_t audio_engine_t::create_voice(const audio_runtime_t& audio, const audio_voice_create_desc_t& desc)
	{
		SFG_ASSERT(_internals != nullptr);
		SFG_ASSERT(desc.resource != NULL_RESOURCE_HANDLE);

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);

		if (internals.voices.is_full())
		{
			SFG_WARN("audio voice pool is full");
			return {};
		}

		const bool streamed = audio.header.storage == audio_storage_e::streamed;

		if (streamed && internals.streamed_voice_count >= internals.streamed_voice_max_count)
		{
			SFG_WARN("streamed audio voice pool is full");
			return {};
		}

		const audio_voice_handle_t handle = internals.voices.emplace();
		audio_voice_t&			   voice  = internals.voices.get(handle);
		voice.resource					  = desc.resource;
		voice.streamed					  = streamed;
		voice.auto_destroy				  = desc.auto_destroy;

		ma_result data_result = MA_ERROR;

		if (streamed)
		{
			char path[96] = {};
			std::snprintf(path, sizeof(path), "sfg_audio:%llu:%llu:%llu", desc.resource, static_cast<u64>(audio.payload_offset), audio.header.payload_size);

			ma_uint32 flags = MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT;

			if (desc.settings.looping)
				flags |= MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_LOOPING;

			data_result = ma_resource_manager_data_source_init(&internals.resource_manager, path, flags, nullptr, &voice.stream_data);

			if (data_result == MA_SUCCESS)
			{
				voice.data_initialized = true;
				internals.streamed_voice_count++;
			}
		}
		else
		{
			ma_audio_buffer_config buffer_config = ma_audio_buffer_config_init(ma_format_s16, audio.header.channels, audio.header.frame_count, audio.resident_data, nullptr);
			buffer_config.sampleRate			 = audio.header.sample_rate;
			data_result							 = ma_audio_buffer_init(&buffer_config, &voice.resident_data);
			voice.data_initialized				 = data_result == MA_SUCCESS;
		}

		if (data_result != MA_SUCCESS)
		{
			SFG_ERR("failed to initialize audio data source: {0}", static_cast<i32>(data_result));
			internals.voices.remove(handle);
			return {};
		}

		ma_data_source* data_source	 = streamed ? reinterpret_cast<ma_data_source*>(&voice.stream_data) : reinterpret_cast<ma_data_source*>(&voice.resident_data);
		const ma_result sound_result = ma_sound_init_from_data_source(&internals.engine, data_source, 0, get_voice_group(internals, desc), &voice.sound);

		if (sound_result != MA_SUCCESS)
		{
			SFG_ERR("failed to initialize audio voice: {0}", static_cast<i32>(sound_result));
			uninit_voice(internals, voice);
			internals.voices.remove(handle);
			return {};
		}

		voice.sound_initialized = true;
		apply_settings(voice, desc.settings);

		return handle;
	}

	void audio_engine_t::destroy_voice(audio_voice_handle_t handle)
	{
		SFG_ASSERT(_internals != nullptr);

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);
		SFG_ASSERT(internals.voices.is_valid(handle));

		audio_voice_t& voice = internals.voices.get(handle);
		uninit_voice(internals, voice);
		internals.voices.remove(handle);
	}

	void audio_engine_t::destroy_voices_for_resource(resource_handle_t resource)
	{
		SFG_ASSERT(_internals != nullptr);

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);

		for (auto it = internals.voices.begin_handle(); it != internals.voices.end_handle();)
		{
			const audio_voice_handle_t handle = *it;
			++it;

			audio_voice_t& voice = internals.voices.get(handle);

			if (voice.resource != resource)
				continue;

			uninit_voice(internals, voice);
			internals.voices.remove(handle);
		}
	}

	bool audio_engine_t::start_voice(audio_voice_handle_t handle)
	{
		SFG_ASSERT(is_voice_valid(handle));

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);
		return ma_sound_start(&internals.voices.get(handle).sound) == MA_SUCCESS;
	}

	bool audio_engine_t::pause_voice(audio_voice_handle_t handle)
	{
		SFG_ASSERT(is_voice_valid(handle));

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);
		return ma_sound_stop(&internals.voices.get(handle).sound) == MA_SUCCESS;
	}

	bool audio_engine_t::stop_voice(audio_voice_handle_t handle)
	{
		SFG_ASSERT(is_voice_valid(handle));

		audio_engine_internals_t& internals	  = *static_cast<audio_engine_internals_t*>(_internals);
		audio_voice_t&			  voice		  = internals.voices.get(handle);
		const ma_result			  stop_result = ma_sound_stop(&voice.sound);
		const ma_result			  seek_result = ma_sound_seek_to_pcm_frame(&voice.sound, 0);
		return stop_result == MA_SUCCESS && seek_result == MA_SUCCESS;
	}

	bool audio_engine_t::seek_voice(audio_voice_handle_t handle, f32 seconds)
	{
		SFG_ASSERT(is_voice_valid(handle));

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);
		return ma_sound_seek_to_second(&internals.voices.get(handle).sound, seconds) == MA_SUCCESS;
	}

	void audio_engine_t::set_voice_settings(audio_voice_handle_t handle, const audio_voice_settings_t& settings)
	{
		SFG_ASSERT(is_voice_valid(handle));

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);
		apply_settings(internals.voices.get(handle), settings);
	}

	void audio_engine_t::set_bus_volume(audio_bus_e bus, f32 volume)
	{
		SFG_ASSERT(_internals != nullptr);
		SFG_ASSERT(bus < audio_bus_e::count);

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);
		ma_sound_group_set_volume(&internals.buses[static_cast<u32>(bus)], volume);
	}

	void audio_engine_t::set_listener(const vec3f_t& position, const vec3f_t& direction, const vec3f_t& velocity)
	{
		SFG_ASSERT(_internals != nullptr);

		audio_engine_internals_t& internals = *static_cast<audio_engine_internals_t*>(_internals);
		ma_engine_listener_set_position(&internals.engine, 0, position.x, position.y, position.z);
		ma_engine_listener_set_direction(&internals.engine, 0, direction.x, direction.y, direction.z);
		ma_engine_listener_set_velocity(&internals.engine, 0, velocity.x, velocity.y, velocity.z);
		ma_engine_listener_set_world_up(&internals.engine, 0, 0.0f, 1.0f, 0.0f);
	}

	bool audio_engine_t::is_voice_valid(audio_voice_handle_t handle) const
	{
		if (_internals == nullptr)
			return false;

		const audio_engine_internals_t& internals = *static_cast<const audio_engine_internals_t*>(_internals);
		return internals.voices.is_valid(handle);
	}

	bool audio_engine_t::is_voice_playing(audio_voice_handle_t handle) const
	{
		SFG_ASSERT(is_voice_valid(handle));

		const audio_engine_internals_t& internals = *static_cast<const audio_engine_internals_t*>(_internals);
		return ma_sound_is_playing(&internals.voices.get(handle).sound) == MA_TRUE;
	}

	bool audio_engine_t::is_voice_at_end(audio_voice_handle_t handle) const
	{
		SFG_ASSERT(is_voice_valid(handle));

		const audio_engine_internals_t& internals = *static_cast<const audio_engine_internals_t*>(_internals);
		return ma_sound_at_end(&internals.voices.get(handle).sound) == MA_TRUE;
	}

	f32 audio_engine_t::get_voice_cursor_seconds(audio_voice_handle_t handle) const
	{
		SFG_ASSERT(is_voice_valid(handle));

		const audio_engine_internals_t& internals = *static_cast<const audio_engine_internals_t*>(_internals);
		f32								seconds	  = 0.0f;
		ma_sound_get_cursor_in_seconds(&internals.voices.get(handle).sound, &seconds);
		return seconds;
	}

	f32 audio_engine_t::get_bus_volume(audio_bus_e bus) const
	{
		SFG_ASSERT(_internals != nullptr);
		SFG_ASSERT(bus < audio_bus_e::count);

		const audio_engine_internals_t& internals = *static_cast<const audio_engine_internals_t*>(_internals);
		return ma_sound_group_get_volume(&internals.buses[static_cast<u32>(bus)]);
	}

	bool audio_engine_t::is_init() const
	{
		return _internals != nullptr;
	}
}
