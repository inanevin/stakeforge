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

#include "data/bitmask.hpp"
#include "data/vector.hpp"
#include "data/span.hpp"
#include "data/string.hpp"
#include "format.hpp"
#include "gfx/common/gfx_constants.hpp"

#ifdef SFG_TOOLMODE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif
#include <io/log.hpp>

namespace SFG
{
	class ostream;
	class istream;

	enum class blend_op : u8
	{
		add,
		subtract,
		reverse_subtract,
		min,
		max
	};

	enum class blend_factor : u16
	{
		zero,
		one,
		src_color,
		one_minus_src_color,
		dst_color,
		one_minus_dst_color,
		src_alpha,
		one_minus_src_alpha,
		dst_alpha,
		one_minus_dst_alpha,
	};

	enum class logic_op : u16
	{
		clear,
		and_,
		and_reverse,
		copy,
		and_inverted,
		no_op,
		xor_,
		or_,
		nor,
		equivalent,
	};

	enum class topology : u8
	{
		point_list,
		line_list,
		line_strip,
		triangle_list,
		triangle_strip,
		triangle_fan,
	};

	enum class fill_mode : u8
	{
		solid,
		wireframe
	};

	enum class polygon_mode : u8
	{
		fill,
		line,
		point
	};

	enum class cull_mode : u8
	{
		none,
		front,
		back,
	};

	enum class front_face : u8
	{
		ccw,
		cw,
	};

	enum class stencil_op : u8
	{
		keep,
		zero,
		replace,
		increment_clamp,
		decrement_clamp,
		invert,
		increment_wrap,
		decrement_wrap,
	};

	enum class load_op : u8
	{
		load,
		clear,
		dont_care,
		none,
	};

	enum class compare_op : u8
	{
		never,
		less,
		equal,
		lequal,
		greater,
		nequal,
		gequal,
		always
	};

	enum class store_op : u8
	{
		store,
		dont_care,
		none,
	};

	enum shader_stage : u8
	{
		vertex,
		fragment,
		compute,
		all
	};

	enum shader_flags
	{
		shf_enable_sample_shading = 1 << 0,
		shf_enable_depth_bias	  = 1 << 1,
		shf_enable_alpha_to_cov	  = 1 << 2,
		shf_enable_blend_logic_op = 1 << 3,
	};

	struct vertex_input
	{
		string name		= "TEXCOORD";
		u8	   location = 0;
		u8	   index	= 0;
		size_t offset	= 0;
		size_t size		= 0;
		format format	= format::undefined;
	};

	struct shader_blob
	{
		shader_stage stage = {};
		span<u8>	 data  = {};
	};

	enum color_comp_flags
	{
		ccf_red	  = 1 << 0,
		ccf_green = 1 << 1,
		ccf_blue  = 1 << 2,
		ccf_alpha = 1 << 3,
		ccf_rgb	  = ccf_red | ccf_green | ccf_blue,
		ccf_rgba  = ccf_red | ccf_green | ccf_blue | ccf_alpha,
	};

	struct color_blend_attachment
	{
		bool		 blend_enabled			= false;
		blend_factor src_color_blend_factor = blend_factor::src_alpha;
		blend_factor dst_color_blend_factor = blend_factor::one_minus_src_alpha;
		blend_op	 color_blend_op			= blend_op::add;
		blend_factor src_alpha_blend_factor = blend_factor::one;
		blend_factor dst_alpha_blend_factor = blend_factor::zero;
		blend_op	 alpha_blend_op			= blend_op::add;
		bitmask<u8>	 color_comp_flags		= ccf_red | ccf_green | ccf_blue | ccf_alpha;
	};

	struct shader_color_attachment
	{
		format				   format			= format::b8g8r8a8_srgb;
		color_blend_attachment blend_attachment = {};
	};

	enum depth_stencil_flags
	{
		dsf_depth_write	   = 1 << 0,
		dsf_depth_test	   = 1 << 1,
		dsf_enable_stencil = 1 << 2,
	};

	struct stencil_state
	{
		stencil_op fail_op		 = stencil_op::keep;
		stencil_op pass_op		 = stencil_op::keep;
		stencil_op depth_fail_op = stencil_op::keep;
		compare_op compare_op	 = compare_op::always;
	};

	struct shader_depth_stencil_desc
	{
		format		  attachment_format	   = format::d32_sfloat;
		compare_op	  depth_compare		   = compare_op::lequal;
		stencil_state back_stencil_state   = {};
		stencil_state front_stencil_state  = {};
		u32			  stencil_compare_mask = 0xFF;
		u32			  stencil_write_mask   = 0xFF;
		bitmask<u8>	  flags				   = 0;
	};

	struct shader_desc
	{
		string							vertex_entry  = "VSMain";
		string							pixel_entry	  = "PSMain";
		string							compute_entry = "CSMain";
		bitmask<u16>					flags		  = 0;
		vector<shader_color_attachment> attachments	  = {};
		vector<vertex_input>			inputs		  = {};

		shader_depth_stencil_desc depth_stencil_desc = {};
		logic_op				  blend_logic_op	 = logic_op::and_;
		topology				  topo				 = topology::triangle_list;
		fill_mode				  fill				 = fill_mode::solid;
		cull_mode				  cull				 = cull_mode::back;
		front_face				  front				 = front_face::cw;
		polygon_mode			  poly_mode			 = polygon_mode::fill;

		u32 samples				= 1;
		f32 depth_bias_constant = 0.0f;
		f32 depth_bias_clamp	= 0.0f;
		f32 depth_bias_slope	= 0.0f;

		string debug_name = "shader";

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

	struct compile_variant
	{
		vector<shader_blob> blobs;
		void				destroy();

		void serialize(ostream& stream, bool address_only = false) const;
		void deserialize(istream& stream, bool address_only = false);
	};

	struct pso_variant
	{
		shader_desc	 desc;
		u32			 compile_variant = 0;
		bitmask<u32> variant_flags	 = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const vertex_input& s);
	void from_json(const nlohmann::json& j, vertex_input& s);

	void to_json(nlohmann::json& j, const shader_desc& s);
	void from_json(const nlohmann::json& j, shader_desc& s);

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

	void to_json(nlohmann::json& j, const color_blend_attachment& att);
	void from_json(const nlohmann::json& j, color_blend_attachment& att);

	void to_json(nlohmann::json& j, const shader_color_attachment& att);
	void from_json(const nlohmann::json& j, shader_color_attachment& att);

	void to_json(nlohmann::json& j, const stencil_state& ss);
	void from_json(const nlohmann::json& j, stencil_state& ss);

	void to_json(nlohmann::json& j, const shader_depth_stencil_desc& att);
	void from_json(const nlohmann::json& j, shader_depth_stencil_desc& att);

#endif
}