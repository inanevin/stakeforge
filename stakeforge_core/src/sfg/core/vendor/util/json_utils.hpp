// Copyright (c) 2025 Inan Evin
#pragma once
#include "core/common/size_definitions.hpp"
#include "core/data/string.hpp"
#include "core/common/sfg_exports.hpp"
#include "core/vendor/nhlohmann/json_fwd.hpp"

namespace sfg
{

	class SFG_API json_util final
	{
	public:
		json_util() = delete;

		static void apply_changes(const nlohmann::json& before, const nlohmann::json& after, nlohmann::json& target);
	};
}
