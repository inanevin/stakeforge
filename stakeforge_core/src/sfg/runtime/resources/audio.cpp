// Copyright (c) 2025 Inan Evin

#include "audio.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"

#include <sfg/audio/audio_engine.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	void audio_header_t::serialize(ostream_t& stream) const
	{
		stream << frame_count << payload_size << sample_rate << channels << storage << encoding << sample_format;
	}

	void audio_header_t::deserialize(istream_t& stream)
	{
		stream >> frame_count >> payload_size >> sample_rate >> channels >> storage >> encoding >> sample_format;
	}

	bool audio_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t header_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, HEADER_WIRE_SIZE, header_stream))
		{
			SFG_ERR("failed to read audio header: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};
		stream.open(header_stream.get_raw(), header_stream.get_size());

		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		audio_runtime_t*   runtime	 = mem.get<audio_runtime_t>(entry.runtime);
		audio_internals_t* internals = mem.get<audio_internals_t>(entry.internals);
		*runtime					 = {};
		*internals					 = {};

		stream >> runtime->header;
		runtime->payload_offset = payload_offset + HEADER_WIRE_SIZE;

		if (runtime->header.storage == audio_storage_e::streamed)
			return true;

		ostream_t payload = {};

		if (!rfs.read_resource(entry.hash, runtime->payload_offset, static_cast<size_t>(runtime->header.payload_size), payload))
		{
			SFG_ERR("failed to read resident audio payload: {0}", entry.hash);
			return false;
		}

		internals->resident_data = mem.allocate_bytes(payload.get_size(), alignof(i16));
		u8* resident_data		 = mem.get<u8>(internals->resident_data);
		SFG_MEMCPY(resident_data, payload.get_raw(), payload.get_size());
		runtime->resident_data = resident_data;

		return true;
	}

	void audio_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		if (audio_engine_t::get().is_init())
			audio_engine_t::get().destroy_voices_for_resource(entry.hash);

		chunk_allocator_t& mem		 = ctx.resource_manager.get_memory();
		audio_runtime_t*   runtime	 = mem.get<audio_runtime_t>(entry.runtime);
		audio_internals_t* internals = mem.get<audio_internals_t>(entry.internals);

		if (internals->resident_data)
			mem.free(internals->resident_data);

		*runtime   = {};
		*internals = {};
	}

	const resource_type_desc_t audio_resource_desc = {
		.type				 = resource_type_e::audio,
		.runtime_size		 = sizeof(audio_runtime_t),
		.runtime_alignment	 = alignof(audio_runtime_t),
		.internals_size		 = sizeof(audio_internals_t),
		.internals_alignment = alignof(audio_internals_t),
		.wire_magic			 = audio_loader_t::WIRE_MAGIC,
		.wire_version		 = audio_loader_t::WIRE_VERSION,
		.load				 = audio_loader_t::load,
		.unload				 = audio_loader_t::unload,
	};
}
