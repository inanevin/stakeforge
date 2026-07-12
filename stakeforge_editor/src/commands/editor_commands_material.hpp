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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/runtime/resources/material_def.hpp>

namespace sfg
{
	struct editor_command_material_edit_payload_t
	{
		chunk_handle32_t material_ids	= {};
		chunk_handle32_t previous_jsons = {};
		chunk_handle32_t post_jsons		= {};
		u32				 count			= 0;
	};

	class editor_command_material_edit_t final
	{
	public:
		editor_command_material_edit_t() = delete;

		static bool edit(span_t<const sid_t> materials, span_t<const material_def_t> previous, span_t<const material_def_t> post);
	};

	class editor_command_shader_edit_t final
	{
	public:
		editor_command_shader_edit_t() = delete;

		static bool edit(span_t<const sid_t> materials, span_t<const material_def_t> previous, span_t<const resource_handle_t> post_shaders);
	};
}
