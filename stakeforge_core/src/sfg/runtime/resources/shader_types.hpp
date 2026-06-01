// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	enum class shader_type_e : u8
	{
		invalid,
		editor_ui_default,
		editor_ui_lcd_text,
		editor_ui_sdf,
		editor_ui_text_grayscale,
		opaque_shader,
		transparent_shader,
		post_process_shader,
		ui_shader,
		ui_text_shader,
		count,
	};

	enum shader_variant_flags_e
	{
		svf_none = 1 << 0,
	};

	void to_json(nlohmann::json& j, const shader_type_e& t);
	void from_json(const nlohmann::json& j, shader_type_e& t);
}
