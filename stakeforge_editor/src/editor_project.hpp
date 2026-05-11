// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct editor_project_t
	{
		string_t path;
		string_t name;
		string_t last_world_resource;

		static bool				is_project_path(const char* path);
		static editor_project_t make_default_project(const char* path);
	};

	void to_json(nlohmann::json& j, const editor_project_t& project);
	void from_json(const nlohmann::json& j, editor_project_t& project);
}
