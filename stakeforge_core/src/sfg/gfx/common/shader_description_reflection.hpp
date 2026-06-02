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

#pragma once

#include "shader_description.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const vertex_input_t& s);
	void from_json(const nlohmann::json& j, vertex_input_t& s);
	void to_json(nlohmann::json& j, const shader_desc_t& s);
	void from_json(const nlohmann::json& j, shader_desc_t& s);
	void to_json(nlohmann::json& j, const cull_mode& c);
	void from_json(const nlohmann::json& j, cull_mode& c);
	void to_json(nlohmann::json& j, const fill_mode& c);
	void from_json(const nlohmann::json& j, fill_mode& c);
	void to_json(nlohmann::json& j, const front_face& f);
	void from_json(const nlohmann::json& j, front_face& f);
	void to_json(nlohmann::json& j, const blend_factor& f);
	void from_json(const nlohmann::json& j, blend_factor& f);
	void to_json(nlohmann::json& j, const blend_op& op);
	void from_json(const nlohmann::json& j, blend_op& op);
	void to_json(nlohmann::json& j, const stencil_op& op);
	void from_json(const nlohmann::json& j, stencil_op& op);
	void to_json(nlohmann::json& j, const compare_op& op);
	void from_json(const nlohmann::json& j, compare_op& op);
	void to_json(nlohmann::json& j, const store_op& op);
	void from_json(const nlohmann::json& j, store_op& op);
	void to_json(nlohmann::json& j, const load_op& op);
	void from_json(const nlohmann::json& j, load_op& op);
	void to_json(nlohmann::json& j, const color_blend_attachment_t& att);
	void from_json(const nlohmann::json& j, color_blend_attachment_t& att);
	void to_json(nlohmann::json& j, const shader_color_attachment_t& att);
	void from_json(const nlohmann::json& j, shader_color_attachment_t& att);
	void to_json(nlohmann::json& j, const stencil_state_t& ss);
	void from_json(const nlohmann::json& j, stencil_state_t& ss);
	void to_json(nlohmann::json& j, const shader_depth_stencil_desc_t& att);
	void from_json(const nlohmann::json& j, shader_depth_stencil_desc_t& att);
}
