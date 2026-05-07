// Copyright (c) 2025 Inan Evin

#include "audio_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool audio_cooker::cook_from_file(const audio_cook_config_t&, const char*, ostream_t&)
	{
		return false;
	}

	void from_json(const nlohmann::json&, audio_cook_config_t&)
	{
	}
}
