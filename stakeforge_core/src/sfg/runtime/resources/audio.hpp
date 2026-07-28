// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include <sfg/common/type_id.hpp>
#include <cstddef>

namespace sfg
{
	enum class audio_storage_e : u8
	{
		resident,
		streamed,
	};

	enum class audio_encoding_e : u8
	{
		pcm_s16,
		mp3,
	};

	enum class audio_sample_format_e : u8
	{
		s16,
	};

	SFG_DEFINE_TYPE_ID(audio_storage_e);
	SFG_DEFINE_TYPE_ID(audio_encoding_e);
	SFG_DEFINE_TYPE_ID(audio_sample_format_e);

	struct audio_header_t
	{
		u64					  frame_count	= 0;
		u64					  payload_size	= 0;
		u32					  sample_rate	= 0;
		u16					  channels		= 0;
		audio_storage_e		  storage		= audio_storage_e::resident;
		audio_encoding_e	  encoding		= audio_encoding_e::pcm_s16;
		audio_sample_format_e sample_format = audio_sample_format_e::s16;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct audio_runtime_t
	{
		const u8*	   resident_data  = nullptr;
		size_t		   payload_offset = 0;
		audio_header_t header		  = {};
	};

	struct audio_internals_t
	{
		chunk_handle32_t resident_data = {};
	};

	class audio_loader_t
	{
	public:
		static constexpr u32 WIRE_MAGIC			 = make_resource_wire_magic('A', 'U', 'D', 'R');
		static constexpr u32 WIRE_VERSION		 = 1;
		static constexpr u32 HEADER_WIRE_SIZE	 = sizeof(u64) * 2 + sizeof(u32) + sizeof(u16) + sizeof(u8) * 3;
		static constexpr u32 RUNTIME_SAMPLE_RATE = 48000;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	extern const resource_type_desc_t audio_resource_desc;
}
