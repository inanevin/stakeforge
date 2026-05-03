// Copyright (c) 2025 Inan Evin

#include "manifest_util.hpp"

namespace sfg
{
	string_t manifest_util::options_to_arguments(const nlohmann::json& options)
	{
		if (options.is_string())
			return options.get<string_t>();

		if (!options.is_object())
			return string_t{};

		string_t result;
		bool	 first = true;
		for (auto it = options.begin(); it != options.end(); ++it)
		{
			if (!first)
				result += ",";
			first = false;
			result += it.key();
			result += "=";
			if (it->is_string())
				result += it->get<string_t>();
			else if (it->is_boolean())
				result += it->get<bool>() ? "true" : "false";
			else if (it->is_number_integer())
				result += std::to_string(it->get<i64>());
			else if (it->is_number())
				result += std::to_string(it->get<double>());
		}
		return result;
	}
}
