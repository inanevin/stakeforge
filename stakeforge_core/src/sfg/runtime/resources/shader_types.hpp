// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/type_id.hpp>

#include <sfg/common/size_definitions.hpp>

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
		deferred_lighting,
		count,
	};

	enum shader_variant_flags_e
	{
		svf_none			   = 0,
		svf_skinned			   = 1 << 0,
		svf_alpha_cutoff	   = 1 << 1,
		svf_z_prepass		   = 1 << 2,
		svf_double_sided	   = 1 << 3,
		svf_shadow_rendering   = 1 << 4,
		svf_selection_outline  = 1 << 5,
		svf_id_write		   = 1 << 6,
		svf_gui_3d			   = 1 << 7,
		svf_console			   = 1 << 8,
		svf_console_and_editor = 1 << 9,
	};

	SFG_DEFINE_TYPE_ID(shader_type_e);

	struct shader_type_reflection_t
	{
		shader_type_reflection_t();
	};

	inline shader_type_reflection_t g_reflect_shader_type;
}
