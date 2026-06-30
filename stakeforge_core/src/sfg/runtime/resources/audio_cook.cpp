// Copyright (c) 2025 Inan Evin

#include "audio_cook.hpp"
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool audio_cooker::cook_from_file(const audio_cook_config_t&, const char*, resource_header_t&, ostream_t&)
	{
		SFG_ERR("audio cooking is not implemented");
		return false;
	}

}

namespace sfg
{
	audio_cook_config_reflection_t::audio_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name	   = "audio_cook_config_t",
			.type_id   = type_id_t<audio_cook_config_t>::value,
			.size	   = sizeof(audio_cook_config_t),
			.alignment = alignof(audio_cook_config_t),
		});
	}
}
