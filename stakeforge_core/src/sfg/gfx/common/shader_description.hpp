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

#include "gfx_constants.hpp"
#include "format.hpp"
#include <sfg/data/bitmask.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	class ostream_t;
	class istream_t;

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

	struct vertex_input_t
	{
		string_t name	  = "TEXCOORD";
		u8		 location = 0;
		u8		 index	  = 0;
		size_t	 offset	  = 0;
		size_t	 size	  = 0;
		format_e format	  = format_e::undefined;
	};

	struct shader_blob_t
	{
		shader_stage stage = {};
		span_t<u8>	 data  = {};
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

	struct color_blend_attachment_t
	{
		bool		  blend_enabled			 = false;
		blend_factor  src_color_blend_factor = blend_factor::src_alpha;
		blend_factor  dst_color_blend_factor = blend_factor::one_minus_src_alpha;
		blend_op	  color_blend_op		 = blend_op::add;
		blend_factor  src_alpha_blend_factor = blend_factor::one;
		blend_factor  dst_alpha_blend_factor = blend_factor::zero;
		blend_op	  alpha_blend_op		 = blend_op::add;
		bitmask_t<u8> color_comp_flags		 = ccf_red | ccf_green | ccf_blue | ccf_alpha;
	};

	struct shader_color_attachment_t
	{
		format_e				 format			  = format_e::b8g8r8a8_srgb;
		color_blend_attachment_t blend_attachment = {};
	};

	enum depth_stencil_flags
	{
		dsf_depth_write	   = 1 << 0,
		dsf_depth_test	   = 1 << 1,
		dsf_enable_stencil = 1 << 2,
	};

	struct stencil_state_t
	{
		stencil_op fail_op		 = stencil_op::keep;
		stencil_op pass_op		 = stencil_op::keep;
		stencil_op depth_fail_op = stencil_op::keep;
		compare_op compare_op	 = compare_op::always;
	};

	struct shader_depth_stencil_desc_t
	{
		format_e		attachment_format	 = format_e::d32_sfloat;
		compare_op		depth_compare		 = compare_op::lequal;
		stencil_state_t back_stencil_state	 = {};
		stencil_state_t front_stencil_state	 = {};
		u32				stencil_compare_mask = 0xFF;
		u32				stencil_write_mask	 = 0xFF;
		bitmask_t<u8>	flags				 = 0;
	};

	struct shader_desc_t
	{
		string_t							vertex_entry  = "VSMain";
		string_t							pixel_entry	  = "PSMain";
		string_t							compute_entry = "CSMain";
		bitmask_t<u16>						flags		  = 0;
		vector_t<shader_color_attachment_t> attachments	  = {};
		vector_t<vertex_input_t>			inputs		  = {};

		shader_depth_stencil_desc_t depth_stencil_desc = {};
		logic_op					blend_logic_op	   = logic_op::and_;
		topology					topo			   = topology::triangle_list;
		fill_mode					fill			   = fill_mode::solid;
		cull_mode					cull			   = cull_mode::back;
		front_face					front			   = front_face::cw;
		polygon_mode				poly_mode		   = polygon_mode::fill;

		u32 samples				= 1;
		f32 depth_bias_constant = 0.0f;
		f32 depth_bias_clamp	= 0.0f;
		f32 depth_bias_slope	= 0.0f;

		string_t debug_name = "shader";

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct compile_variant_t
	{
		vector_t<shader_blob_t> blobs;
		void					destroy();

		void serialize(ostream_t& stream, bool address_only = false) const;
		void deserialize(istream_t& stream, bool address_only = false);
	};

	struct pso_variant_t
	{
		shader_desc_t  desc;
		u32			   compile_variant_t = 0;
		bitmask_t<u32> variant_flags	 = 0;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

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
