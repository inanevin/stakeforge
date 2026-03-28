/*
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
#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "data/span.hpp"
#include "data/vector.hpp"

namespace SFG
{
	struct shader_raw;
	struct shader_desc;

	class shader_variant_compiler
	{
	public:
		struct compile_params
		{
			u8				  stage;
			span<u8>&		  data;
			const vector<string>& defines;
			const string&		  text;
			const vector<string>& folder_paths;
			bool				  compile_layout;
			span<u8>&		  out_layout;
			const string&		  entry;
		};

		/* actual compilation */
		static bool compile(const compile_params& p);
		static bool compile_compute(const compile_params& p);

		/* compiles default pso based on desc. */
		static bool compile_raw(shader_raw& raw, const string& shader_text, const vector<string>& folder_path, const shader_desc& desc);

		/* compiles variants suitable to use in gbuffer pass. */
		static bool compile_style_gbuffer_object(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants suitable to use in forward transparency pass. */
		static bool compile_style_forward_object(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for object outline pass. */
		static bool compile_style_object_outline(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for post combiner. */
		static bool compile_style_post_combiner(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for post combiner. */
		static bool compile_style_gizmo(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for forward pass debug */
		static bool compile_style_debug_triangle(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);
		static bool compile_style_debug_line(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for 2d & 3d gui */
		static bool compile_style_gui(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for engine gui */
		static bool compile_style_engine_gui(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for particles */
		static bool compile_style_particle_additive(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* compiles variants for sprites */
		static bool compile_style_sprite(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);

		/* swapchain */
		static bool compile_style_swapchain(shader_raw& raw, const string& shader_text, const vector<string>& folder_path);
	};
}
