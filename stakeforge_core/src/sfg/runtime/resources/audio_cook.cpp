// Copyright (c) 2025 Inan Evin

#include "audio_cook.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool audio_cooker::cook_from_file(const audio_cook_config_t&, const char*, ostream_t&)
	{
		return false;
	}

}

namespace sfg
{
	audio_cook_config_reflection_t::audio_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<audio_cook_config_t>::value) != nullptr)
			return;

		registry.register_type({
			.name	   = "audio_cook_config_t",
			.type_id   = type_id_t<audio_cook_config_t>::value,
			.size	   = sizeof(audio_cook_config_t),
			.alignment = alignof(audio_cook_config_t),
		});
	}
}
