// Copyright (c) 2025 Inan Evin

#include "audio_cook.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/miniaudio/miniaudio.h>

#include <limits>

namespace sfg
{
	bool audio_cooker::cook_from_file(const audio_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		const ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_s16, 0, audio_loader_t::RUNTIME_SAMPLE_RATE);
		ma_decoder				decoder		   = {};
		const ma_result			init_result	   = ma_decoder_init_file(full_path, &decoder_config, &decoder);

		if (init_result != MA_SUCCESS)
		{
			SFG_ERR("failed to initialize MP3 decoder for {0}: {1}", full_path, static_cast<i32>(init_result));
			return false;
		}

		ma_uint64		frame_count	  = 0;
		const ma_result length_result = ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);

		if (length_result != MA_SUCCESS || frame_count == 0)
		{
			SFG_ERR("failed to read MP3 frame count for {0}: {1}", full_path, static_cast<i32>(length_result));
			ma_decoder_uninit(&decoder);
			return false;
		}

		const u32 channels	  = decoder.outputChannels;
		const u32 sample_rate = decoder.outputSampleRate;

		if (channels == 0 || channels > std::numeric_limits<u16>::max())
		{
			SFG_ERR("invalid MP3 channel count for {0}: {1}", full_path, channels);
			ma_decoder_uninit(&decoder);
			return false;
		}

		audio_header_t audio_header = {
			.frame_count   = frame_count,
			.sample_rate   = sample_rate,
			.channels	   = static_cast<u16>(channels),
			.storage	   = cfg.storage,
			.encoding	   = cfg.storage == audio_storage_e::resident ? audio_encoding_e::pcm_s16 : audio_encoding_e::mp3,
			.sample_format = audio_sample_format_e::s16,
		};

		out_header = {
			.type		 = resource_type_e::audio,
			.magic		 = audio_loader_t::WIRE_MAGIC,
			.version	 = audio_loader_t::WIRE_VERSION,
			.source_tick = file_system_t::get_last_modified_ticks(full_path),
		};

		if (cfg.storage == audio_storage_e::streamed)
		{
			ma_decoder_uninit(&decoder);

			istream_t source = serializer_t::load_from_file(full_path);

			if (source.empty())
			{
				SFG_ERR("failed to read streamed MP3 source: {0}", full_path);
				return false;
			}

			audio_header.payload_size = source.get_size();
			stream << audio_header;
			stream.write_raw(source.get_raw(), source.get_size());

			return true;
		}

		const u64 bytes_per_frame = sizeof(i16) * channels;

		if (frame_count > std::numeric_limits<size_t>::max() / bytes_per_frame)
		{
			SFG_ERR("resident MP3 payload is too large: {0}", full_path);
			ma_decoder_uninit(&decoder);
			return false;
		}

		const size_t pcm_size = static_cast<size_t>(frame_count * bytes_per_frame);
		u8*			 pcm_data = static_cast<u8*>(SFG_MALLOC(pcm_size));

		if (pcm_data == nullptr)
		{
			SFG_ERR("failed to allocate decoded MP3 payload: {0}", full_path);
			ma_decoder_uninit(&decoder);
			return false;
		}

		ma_uint64		frames_read = 0;
		const ma_result read_result = ma_decoder_read_pcm_frames(&decoder, pcm_data, frame_count, &frames_read);
		ma_decoder_uninit(&decoder);

		if (read_result != MA_SUCCESS || frames_read != frame_count)
		{
			SFG_ERR("failed to decode MP3 payload for {0}: {1}", full_path, static_cast<i32>(read_result));
			SFG_FREE(pcm_data);
			return false;
		}

		audio_header.payload_size = pcm_size;
		stream << audio_header;
		stream.write_raw(pcm_data, pcm_size);
		SFG_FREE(pcm_data);

		return true;
	}

	audio_cook_config_reflection_t::audio_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "audio_storage_e",
			.display_name = "Audio Storage",
			.fields =
				{
					{.name = "resident", .display_name = "Resident"},
					{.name = "streamed", .display_name = "Streamed"},
				},
			.type_id   = type_id_t<audio_storage_e>::value,
			.size	   = sizeof(audio_storage_e),
			.alignment = alignof(audio_storage_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name		  = "audio_cook_config_t",
			.display_name = "Audio Cook Config",
			.fields =
				{
					{.name = "storage", .display_name = "Storage", .sub_type_id = type_id_t<audio_storage_e>::value, .offset = offsetof(audio_cook_config_t, storage), .size = sizeof(audio_storage_e), .type = reflected_value_type_e::u8},
				},
			.type_id   = type_id_t<audio_cook_config_t>::value,
			.size	   = sizeof(audio_cook_config_t),
			.alignment = alignof(audio_cook_config_t),
		});
	}
}
