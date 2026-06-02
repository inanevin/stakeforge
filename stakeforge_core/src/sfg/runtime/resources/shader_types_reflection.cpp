/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "shader_types_reflection.hpp"
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const shader_type_e& t)
		{
			switch (t)
			{
			case shader_type_e::opaque_shader:
				j = "opaque_shader";
				break;
			case shader_type_e::transparent_shader:
				j = "transparent_shader";
				break;
			case shader_type_e::post_process_shader:
				j = "post_process_shader";
				break;
			case shader_type_e::ui_shader:
				j = "ui_shader";
				break;
			case shader_type_e::ui_text_shader:
				j = "ui_text_shader";
				break;
			case shader_type_e::editor_ui_default:
				j = "editor_ui_default";
				break;
			case shader_type_e::editor_ui_lcd_text:
				j = "editor_ui_lcd_text";
				break;
			case shader_type_e::editor_ui_text_grayscale:
				j = "editor_ui_text_grayscale";
				break;
			case shader_type_e::editor_ui_sdf:
				j = "editor_ui_sdf";
				break;
			default:
				j = "invalid";
				break;
			}
		}

	void from_json(const nlohmann::json& j, shader_type_e& t)
		{
			const string_t s = j.get<string_t>();
	
			if (s == "opaque_shader")
				t = shader_type_e::opaque_shader;
			else if (s == "transparent_shader")
				t = shader_type_e::transparent_shader;
			else if (s == "post_process_shader")
				t = shader_type_e::post_process_shader;
			else if (s == "ui_shader")
				t = shader_type_e::ui_shader;
			else if (s == "ui_text_shader")
				t = shader_type_e::ui_text_shader;
			else if (s == "editor_ui_default")
				t = shader_type_e::editor_ui_default;
			else if (s == "editor_ui_lcd_text")
				t = shader_type_e::editor_ui_lcd_text;
			else if (s == "editor_ui_text_grayscale")
				t = shader_type_e::editor_ui_text_grayscale;
			else if (s == "editor_ui_sdf")
				t = shader_type_e::editor_ui_sdf;
			else
				t = shader_type_e::invalid;
		}

}
