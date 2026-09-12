// Copyright (c) 2025 Inan Evin

#include "animation.hpp"
#include "animation_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/memory/bump_allocator.hpp>
#include <sfg/serialization/compression.hpp>

namespace sfg
{
	bool animation_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read animation resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());
		istream_t payload = compressor_t::decompress(stream);

		if (payload.empty())
		{
			SFG_ERR("failed to decompress animation payload: {0}", entry.hash);
			return false;
		}

		chunk_allocator_t&	 memory		= ctx.resource_manager.get_memory();
		animation_runtime_t* runtime	= memory.get<animation_runtime_t>(entry.runtime);
		animation_def_t		 definition = {};

		*runtime = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_def_t>::value, &definition, nullptr, payload))
		{
			SFG_ERR("failed to deserialize animation definition: {0}", entry.hash);
			return false;
		}

		size_t storage_size = 0;

		const auto measure_channels = [&storage_size]<typename runtime_channel_t>(const auto& channels) {
			if (!channels.empty())
				storage_size = (ALIGN_UP(storage_size, alignof(runtime_channel_t))) + channels.size() * sizeof(runtime_channel_t);

			for (const auto& channel : channels)
			{
				if (!channel.keyframes.empty())
					storage_size = (ALIGN_UP(storage_size, alignof(std::remove_cvref_t<decltype(channel.keyframes[0])>))) + channel.keyframes.size() * sizeof(channel.keyframes[0]);

				if (!channel.keyframes_spline.empty())
					storage_size = (ALIGN_UP(storage_size, alignof(std::remove_cvref_t<decltype(channel.keyframes_spline[0])>))) + channel.keyframes_spline.size() * sizeof(channel.keyframes_spline[0]);
			}
		};

		measure_channels.operator()<animation_channel_v3_runtime_t>(definition.position_channels);
		measure_channels.operator()<animation_channel_q_runtime_t>(definition.rotation_channels);
		measure_channels.operator()<animation_channel_v3_runtime_t>(definition.scale_channels);

		runtime->preview_mesh	  = definition.preview_mesh;
		runtime->preview_skeleton = definition.preview_skeleton;
		runtime->duration		  = definition.duration;

		if (!definition.events.empty())
			storage_size = (ALIGN_UP(storage_size, alignof(animation_event_t))) + definition.events.size() * sizeof(animation_event_t);

		if (storage_size == 0)
		{
			SFG_ERR("animation has no channels or events: {0}", entry.hash);
			return false;
		}

		runtime->data = memory.allocate_bytes(storage_size, alignof(std::max_align_t));

		bump_allocator_t storage = {};

		storage.init(memory.get<u8>(runtime->data), storage_size);

		const auto load_channels = [&storage]<typename runtime_channel_t>(const auto& channels) {
			runtime_channel_t* output = storage.allocate<runtime_channel_t>(channels.size());

			for (u32 index = 0; index < channels.size(); ++index)
			{
				const auto& source	  = channels[index];
				using keyframe_t	  = std::remove_cvref_t<decltype(source.keyframes[0])>;
				using spline_t		  = std::remove_cvref_t<decltype(source.keyframes_spline[0])>;
				keyframe_t* keyframes = storage.allocate<keyframe_t>(source.keyframes.size());
				spline_t*	splines	  = storage.allocate<spline_t>(source.keyframes_spline.size());

				if (!source.keyframes.empty())
					SFG_MEMCPY(keyframes, source.keyframes.data(), source.keyframes.size() * sizeof(keyframe_t));

				if (!source.keyframes_spline.empty())
					SFG_MEMCPY(splines, source.keyframes_spline.data(), source.keyframes_spline.size() * sizeof(spline_t));

				output[index] = {
					.keyframes		  = keyframes,
					.keyframes_spline = splines,
					.keyframe_count	  = static_cast<u32>(source.keyframes.size()),
					.spline_count	  = static_cast<u32>(source.keyframes_spline.size()),
					.node_index		  = source.node_index,
					.interpolation	  = source.interpolation,
				};
			}

			return output;
		};

		runtime->position_channels = load_channels.operator()<animation_channel_v3_runtime_t>(definition.position_channels);
		runtime->rotation_channels = load_channels.operator()<animation_channel_q_runtime_t>(definition.rotation_channels);
		runtime->scale_channels	   = load_channels.operator()<animation_channel_v3_runtime_t>(definition.scale_channels);
		runtime->position_count	   = static_cast<u32>(definition.position_channels.size());
		runtime->rotation_count	   = static_cast<u32>(definition.rotation_channels.size());
		runtime->scale_count	   = static_cast<u32>(definition.scale_channels.size());
		runtime->event_count	   = static_cast<u32>(definition.events.size());

		animation_event_t* events = storage.allocate<animation_event_t>(runtime->event_count);

		for (u32 index = 0; index < runtime->event_count; ++index)
			events[index] = {.name_hash = TO_SID(static_cast<const char*>(definition.events[index].name)), .time = definition.events[index].time};

		if (runtime->event_count != 0)
			std::sort(events, events + runtime->event_count, [](const animation_event_t& a, const animation_event_t& b) { return a.time < b.time; });

		runtime->events = events;
		storage.uninit();

		return true;
	}

	void animation_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	 memory	 = ctx.resource_manager.get_memory();
		animation_runtime_t* runtime = memory.get<animation_runtime_t>(entry.runtime);

		if (runtime->data)
			memory.free(runtime->data);

		*runtime = {};
	}

	const resource_type_desc_t animation_resource_desc = {
		.type				 = resource_type_e::animation,
		.runtime_size		 = sizeof(animation_runtime_t),
		.runtime_alignment	 = alignof(animation_runtime_t),
		.internals_size		 = sizeof(animation_internals_t),
		.internals_alignment = alignof(animation_internals_t),
		.wire_magic			 = animation_loader_t::WIRE_MAGIC,
		.wire_version		 = animation_loader_t::WIRE_VERSION,
		.load				 = animation_loader_t::load,
		.unload				 = animation_loader_t::unload,
	};
}
