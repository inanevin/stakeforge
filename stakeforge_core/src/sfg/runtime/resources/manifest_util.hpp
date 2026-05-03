// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace manifest_util
	{
		string_t options_to_arguments(const nlohmann::json& options);
	}
}
