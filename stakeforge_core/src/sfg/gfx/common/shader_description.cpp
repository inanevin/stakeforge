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

#include "shader_description.hpp"
#include "io/assert.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#include "vendor/nhlohmann/json.hpp"
using json = nlohmann::json;

namespace sfg
{
	void to_json(nlohmann::json& j, const vertex_input_t& s)
	{
		j["name"]	  = s.name;
		j["location"] = s.location;
		j["index"]	  = s.index;
		j["offset"]	  = s.offset;
		j["size"]	  = s.size;
		j["format"]	  = s.format;
	}

	void from_json(const nlohmann::json& j, vertex_input_t& s)
	{
		s.name	   = j.value<string_t>("name", "");
		s.location = j.value<u8>("location", 0);
		s.index	   = j.value<u8>("index", 0);
		s.offset   = j.value<u8>("offset", 0);
		s.size	   = j.value<u32>("size", 0);
		s.format   = j.value<format_t>("format", format_t::undefined);
	}

	void to_json(nlohmann::json& j, const shader_desc_t& s)
	{
		j["pixel_entry"]		 = s.pixel_entry;
		j["vertex_entry"]		 = s.vertex_entry;
		j["compute_entry"]		 = s.compute_entry;
		j["cull"]				 = s.cull;
		j["front"]				 = s.front;
		j["vertex_inputs"]		 = s.inputs;
		j["color_attachments"]	 = s.attachments;
		j["samples"]			 = s.samples;
		j["depth_stencil"]		 = s.depth_stencil_desc;
		j["fill"]				 = s.fill;
		j["topology"]			 = s.topo;
		j["poly_mode"]			 = s.poly_mode;
		j["depth_bias_constant"] = s.depth_bias_constant;
		j["depth_bias_clamp"]	 = s.depth_bias_clamp;
		j["depth_bias_slope"]	 = s.depth_bias_slope;
	}

	void from_json(const nlohmann::json& j, shader_desc_t& s)
	{
		s.pixel_entry		  = j.value<string_t>("pixel_entry", "");
		s.vertex_entry		  = j.value<string_t>("vertex_entry", "");
		s.compute_entry		  = j.value<string_t>("compute_entry", "");
		s.cull				  = j.value<cull_mode>("cull", cull_mode::back);
		s.front				  = j.value<front_face>("front", front_face::ccw);
		s.fill				  = j.value<fill_mode>("fill", fill_mode::solid);
		s.poly_mode			  = j.value<polygon_mode>("poly_mode", polygon_mode::fill);
		s.topo				  = j.value<topology>("topology", topology::triangle_list);
		s.inputs			  = j.value<vector_t<vertex_input_t>>("vertex_inputs", {});
		s.attachments		  = j.value<vector_t<shader_color_attachment_t>>("color_attachments", {});
		s.samples			  = j.value<u32>("samples", 1);
		s.depth_stencil_desc  = j.value<shader_depth_stencil_desc_t>("depth_stencil", {});
		s.depth_bias_constant = j.value<f32>("depth_bias_constant", 0.0f);
		s.depth_bias_slope	  = j.value<f32>("depth_bias_slope", 0.0f);
		s.depth_bias_clamp	  = j.value<f32>("depth_bias_clamp", 0.0f);
	}

	void to_json(nlohmann::json& j, const cull_mode& c)
	{
		switch (c)
		{
		case cull_mode::none:
			j = "none";
			break;
		case cull_mode::front:
			j = "front";
			break;
		case cull_mode::back:
			j = "back";
			break;
		}
	}

	void from_json(const nlohmann::json& j, cull_mode& c)
	{
		const string_t& str = j.get<string_t>();
		if (str.compare("none") == 0)
		{
			c = cull_mode::none;
			return;
		}

		if (str.compare("back") == 0)
		{
			c = cull_mode::back;
			return;
		}
		if (str.compare("front") == 0)
		{
			c = cull_mode::front;
			return;
		}

		SFG_ASSERT(false);
	}

	void to_json(nlohmann::json& j, const fill_mode& c)
	{
		switch (c)
		{
		case fill_mode::solid:
			j = "solid";
			break;
		case fill_mode::wireframe:
			j = "wireframe";
			break;
		}
	}

	void from_json(const nlohmann::json& j, fill_mode& c)
	{
		const string_t& str = j.get<string_t>();

		if (str.compare("wireframe") == 0)
		{
			c = fill_mode::wireframe;
			return;
		}

		c = fill_mode::solid;
	}

	void to_json(nlohmann::json& j, const front_face& f)
	{
		switch (f)
		{
		case front_face::ccw:
			j = "ccw";
			break;
		case front_face::cw:
			j = "cw";
			break;
		}
	}

	void from_json(const nlohmann::json& j, front_face& f)
	{
		const string_t& str = j.get<string_t>();
		if (str.compare("cw") == 0)
		{
			f = front_face::cw;
			return;
		}

		if (str.compare("ccw") == 0)
		{
			f = front_face::ccw;
			return;
		}

		SFG_ASSERT(false);
	}

	void to_json(nlohmann::json& j, const blend_factor& f)
	{
		switch (f)
		{
		case blend_factor::zero:
			j = "zero";
			break;
		case blend_factor::one:
			j = "one";
			break;
		case blend_factor::src_color:
			j = "src_color";
			break;
		case blend_factor::one_minus_src_color:
			j = "one_minus_src_color";
			break;
		case blend_factor::dst_color:
			j = "dst_color";
			break;
		case blend_factor::one_minus_dst_color:
			j = "one_minus_dst_color";
			break;
		case blend_factor::src_alpha:
			j = "src_alpha";
			break;
		case blend_factor::one_minus_src_alpha:
			j = "one_minus_src_alpha";
			break;
		case blend_factor::dst_alpha:
			j = "dst_alpha";
			break;
		case blend_factor::one_minus_dst_alpha:
			j = "one_minus_dst_alpha";
			break;
		}
	}

	void from_json(const nlohmann::json& j, blend_factor& f)
	{
		const string_t& str = j.get<string_t>();

		if (str.compare("zero") == 0)
		{
			f = blend_factor::zero;
			return;
		}

		if (str.compare("one") == 0)
		{
			f = blend_factor::one;
			return;
		}

		if (str.compare("src_color") == 0)
		{
			f = blend_factor::src_color;
			return;
		}

		if (str.compare("one_minus_src_color") == 0)
		{
			f = blend_factor::one_minus_src_color;
			return;
		}

		if (str.compare("dst_color") == 0)
		{
			f = blend_factor::dst_color;
			return;
		}

		if (str.compare("one_minus_dst_color") == 0)
		{
			f = blend_factor::one_minus_dst_color;
			return;
		}

		if (str.compare("src_alpha") == 0)
		{
			f = blend_factor::src_alpha;
			return;
		}

		if (str.compare("one_minus_src_alpha") == 0)
		{
			f = blend_factor::one_minus_src_alpha;
			return;
		}

		if (str.compare("dst_alpha") == 0)
		{
			f = blend_factor::dst_alpha;
			return;
		}

		if (str.compare("one_minus_dst_alpha") == 0)
		{
			f = blend_factor::one_minus_dst_alpha;
			return;
		}

		SFG_ASSERT(false);
	}

	void to_json(nlohmann::json& j, const blend_op& op)
	{
		switch (op)
		{
		case blend_op::add:
			j = "add";
			break;
		case blend_op::subtract:
			j = "subtract";
			break;
		case blend_op::reverse_subtract:
			j = "reverse_subtract";
			break;
		case blend_op::min:
			j = "min";
			break;
		case blend_op::max:
			j = "max";
			break;
		}
	}

	void from_json(const nlohmann::json& j, blend_op& op)
	{
		const string_t& str = j.get<string_t>();

		if (str.compare("add") == 0)
		{
			op = blend_op::add;
			return;
		}

		if (str.compare("subtract") == 0)
		{
			op = blend_op::subtract;
			return;
		}

		if (str.compare("reverse_subtract") == 0)
		{
			op = blend_op::reverse_subtract;
			return;
		}

		if (str.compare("min") == 0)
		{
			op = blend_op::min;
			return;
		}

		if (str.compare("max") == 0)
		{
			op = blend_op::max;
			return;
		}

		SFG_ASSERT(false);
	}

	// --- stencil_op ---
	void to_json(nlohmann::json& j, const stencil_op& op)
	{
		switch (op)
		{
		case stencil_op::keep:
			j = "keep";
			break;
		case stencil_op::zero:
			j = "zero";
			break;
		case stencil_op::replace:
			j = "replace";
			break;
		case stencil_op::increment_clamp:
			j = "increment_clamp";
			break;
		case stencil_op::decrement_clamp:
			j = "decrement_clamp";
			break;
		case stencil_op::invert:
			j = "invert";
			break;
		case stencil_op::increment_wrap:
			j = "increment_wrap";
			break;
		case stencil_op::decrement_wrap:
			j = "decrement_wrap";
			break;
		}
	}

	void from_json(const nlohmann::json& j, stencil_op& op)
	{
		const std::string& str = j.get<std::string>();

		if (str.compare("keep") == 0)
		{
			op = stencil_op::keep;
			return;
		}
		if (str.compare("zero") == 0)
		{
			op = stencil_op::zero;
			return;
		}
		if (str.compare("replace") == 0)
		{
			op = stencil_op::replace;
			return;
		}
		if (str.compare("increment_clamp") == 0)
		{
			op = stencil_op::increment_clamp;
			return;
		}
		if (str.compare("decrement_clamp") == 0)
		{
			op = stencil_op::decrement_clamp;
			return;
		}
		if (str.compare("invert") == 0)
		{
			op = stencil_op::invert;
			return;
		}
		if (str.compare("increment_wrap") == 0)
		{
			op = stencil_op::increment_wrap;
			return;
		}
		if (str.compare("decrement_wrap") == 0)
		{
			op = stencil_op::decrement_wrap;
			return;
		}

		SFG_ASSERT(false);
	}

	// --- load_op ---
	void to_json(nlohmann::json& j, const load_op& op)
	{
		switch (op)
		{
		case load_op::load:
			j = "load";
			break;
		case load_op::clear:
			j = "clear";
			break;
		case load_op::dont_care:
			j = "dont_care";
			break;
		case load_op::none:
			j = "none";
			break;
		}
	}

	void from_json(const nlohmann::json& j, load_op& op)
	{
		const std::string& str = j.get<std::string>();

		if (str.compare("load") == 0)
		{
			op = load_op::load;
			return;
		}
		if (str.compare("clear") == 0)
		{
			op = load_op::clear;
			return;
		}
		if (str.compare("dont_care") == 0)
		{
			op = load_op::dont_care;
			return;
		}
		if (str.compare("none") == 0)
		{
			op = load_op::none;
			return;
		}

		SFG_ASSERT(false);
	}

	// --- compare_op ---
	void to_json(nlohmann::json& j, const compare_op& op)
	{
		switch (op)
		{
		case compare_op::never:
			j = "never";
			break;
		case compare_op::less:
			j = "less";
			break;
		case compare_op::equal:
			j = "equal";
			break;
		case compare_op::lequal:
			j = "lequal";
			break;
		case compare_op::greater:
			j = "greater";
			break;
		case compare_op::nequal:
			j = "nequal";
			break;
		case compare_op::gequal:
			j = "gequal";
			break;
		case compare_op::always:
			j = "always";
			break;
		}
	}

	void from_json(const nlohmann::json& j, compare_op& op)
	{
		const std::string& str = j.get<std::string>();

		if (str.compare("never") == 0)
		{
			op = compare_op::never;
			return;
		}
		if (str.compare("less") == 0)
		{
			op = compare_op::less;
			return;
		}
		if (str.compare("equal") == 0)
		{
			op = compare_op::equal;
			return;
		}
		if (str.compare("lequal") == 0)
		{
			op = compare_op::lequal;
			return;
		}
		if (str.compare("greater") == 0)
		{
			op = compare_op::greater;
			return;
		}
		if (str.compare("nequal") == 0)
		{
			op = compare_op::nequal;
			return;
		}
		if (str.compare("gequal") == 0)
		{
			op = compare_op::gequal;
			return;
		}
		if (str.compare("always") == 0)
		{
			op = compare_op::always;
			return;
		}

		SFG_ASSERT(false);
	}

	// --- store_op ---
	void to_json(nlohmann::json& j, const store_op& op)
	{
		switch (op)
		{
		case store_op::store:
			j = "store";
			break;
		case store_op::dont_care:
			j = "dont_care";
			break;
		case store_op::none:
			j = "none";
			break;
		}
	}

	void from_json(const nlohmann::json& j, store_op& op)
	{
		const std::string& str = j.get<std::string>();

		if (str.compare("store") == 0)
		{
			op = store_op::store;
			return;
		}
		if (str.compare("dont_care") == 0)
		{
			op = store_op::dont_care;
			return;
		}
		if (str.compare("none") == 0)
		{
			op = store_op::none;
			return;
		}

		SFG_ASSERT(false);
	}

	void to_json(nlohmann::json& j, const color_blend_attachment_t& att)
	{
		j["blend_enabled"]			= att.blend_enabled;
		j["src_color_blend_factor"] = att.src_color_blend_factor;
		j["dst_color_blend_factor"] = att.dst_color_blend_factor;
		j["color_blend_op"]			= att.color_blend_op;
		j["src_alpha_blend_factor"] = att.src_alpha_blend_factor;
		j["dst_alpha_blend_factor"] = att.dst_alpha_blend_factor;
		j["alpha_blend_op"]			= att.alpha_blend_op;
		j["color_comp_flags"]		= att.color_comp_flags.value();
	}

	void from_json(const nlohmann::json& j, color_blend_attachment_t& att)
	{
		att.blend_enabled		   = j.value("blend_enabled", false);
		att.src_color_blend_factor = j.value("src_color_blend_factor", att.src_color_blend_factor);
		att.dst_color_blend_factor = j.value("dst_color_blend_factor", att.dst_color_blend_factor);
		att.color_blend_op		   = j.value("color_blend_op", att.color_blend_op);
		att.src_alpha_blend_factor = j.value("src_alpha_blend_factor", att.src_alpha_blend_factor);
		att.dst_alpha_blend_factor = j.value("dst_alpha_blend_factor", att.dst_alpha_blend_factor);
		att.alpha_blend_op		   = j.value("alpha_blend_op", att.alpha_blend_op);

		const u8 color_t	 = j.value<u8>("color_comp_flags", color_comp_flags::ccf_rgba);
		att.color_comp_flags = color_t;
	}

	void to_json(nlohmann::json& j, const shader_color_attachment_t& att)
	{
		j["format"]			  = att.format;
		j["blend_attachment"] = att.blend_attachment;
	}

	void from_json(const nlohmann::json& j, shader_color_attachment_t& att)
	{
		att.format			 = j.value<format_t>("format", format_t::undefined);
		att.blend_attachment = j.value<color_blend_attachment_t>("blend_attachment", {});
	}

	void to_json(nlohmann::json& j, const stencil_state_t& ss)
	{
		j["compare_op"]	   = ss.compare_op;
		j["depth_fail_op"] = ss.depth_fail_op;
		j["fail_op"]	   = ss.fail_op;
		j["pass_op"]	   = ss.pass_op;
	}

	void from_json(const nlohmann::json& j, stencil_state_t& ss)
	{
		ss.compare_op	 = j.value("compare_op", compare_op::always);
		ss.depth_fail_op = j.value("depth_fail_op", stencil_op::keep);
		ss.fail_op		 = j.value("fail_op", stencil_op::keep);
		ss.pass_op		 = j.value("pass_op", stencil_op::keep);
	}

	void to_json(nlohmann::json& j, const shader_depth_stencil_desc_t& att)
	{
		j["attachment_format"]	  = att.attachment_format;
		j["depth_compare"]		  = att.depth_compare;
		j["back_stencil_state"]	  = att.back_stencil_state;
		j["front_stencil_state"]  = att.front_stencil_state;
		j["stencil_compare_mask"] = att.stencil_compare_mask;
		j["stencil_write_mask"]	  = att.stencil_write_mask;
		j["depth_write"]		  = att.flags.is_set(depth_stencil_flags::dsf_depth_write) ? 1 : 0;
		j["depth_test"]			  = att.flags.is_set(depth_stencil_flags::dsf_depth_test) ? 1 : 0;
	}

	void from_json(const nlohmann::json& j, shader_depth_stencil_desc_t& att)
	{
		att.attachment_format	 = j.value("attachment_format", format_t::d32_sfloat);
		att.depth_compare		 = j.value("depth_compare", compare_op::lequal);
		att.back_stencil_state	 = j.value("back_stencil_state", stencil_state_t{});
		att.front_stencil_state	 = j.value("front_stencil_state", stencil_state_t{});
		att.stencil_compare_mask = j.value("stencil_compare_mask", static_cast<u32>(0xFF));
		att.stencil_write_mask	 = j.value("stencil_write_mask", static_cast<u32>(0xFF));
		const u8 depth_write	 = j.value<u8>("depth_write", 0);
		const u8 depth_test		 = j.value<u8>("depth_test", 0);
		att.flags.set(depth_stencil_flags::dsf_depth_write, depth_write);
		att.flags.set(depth_stencil_flags::dsf_depth_test, depth_test);
	}

	void shader_desc_t::serialize(ostream_t& stream) const
	{
		stream << debug_name;
		stream << vertex_entry;
		stream << pixel_entry;
		stream << compute_entry;
		stream << flags.value();

		const u16 att_count = static_cast<u16>(attachments.size());
		stream << att_count;

		for (const shader_color_attachment_t& att : attachments)
		{
			stream << att.format;
			stream << att.blend_attachment.blend_enabled;
			stream << att.blend_attachment.src_alpha_blend_factor;
			stream << att.blend_attachment.dst_alpha_blend_factor;
			stream << att.blend_attachment.color_blend_op;
			stream << att.blend_attachment.src_alpha_blend_factor;
			stream << att.blend_attachment.dst_alpha_blend_factor;
			stream << att.blend_attachment.alpha_blend_op;
			stream << att.blend_attachment.color_comp_flags.value();
		}

		// depth stencil
		{
			stream << depth_stencil_desc.attachment_format;
			stream << depth_stencil_desc.depth_compare;
			stream << depth_stencil_desc.back_stencil_state.compare_op;
			stream << depth_stencil_desc.back_stencil_state.depth_fail_op;
			stream << depth_stencil_desc.back_stencil_state.fail_op;
			stream << depth_stencil_desc.back_stencil_state.pass_op;
			stream << depth_stencil_desc.front_stencil_state.compare_op;
			stream << depth_stencil_desc.front_stencil_state.depth_fail_op;
			stream << depth_stencil_desc.front_stencil_state.fail_op;
			stream << depth_stencil_desc.front_stencil_state.pass_op;
			stream << depth_stencil_desc.stencil_compare_mask;
			stream << depth_stencil_desc.stencil_write_mask;
			stream << depth_stencil_desc.flags.value();
			stream << depth_bias_clamp;
			stream << depth_bias_constant;
			stream << depth_bias_slope;
		}

		const u16 inp_count = static_cast<u16>(inputs.size());
		stream << inp_count;

		for (const vertex_input_t& inp : inputs)
		{
			stream << inp.name;
			stream << inp.location;
			stream << inp.index;
			stream << static_cast<u32>(inp.offset);
			stream << static_cast<u32>(inp.size);
			stream << inp.format;
		}

		stream << fill;
		stream << blend_logic_op;
		stream << topo;
		stream << cull;
		stream << front;
		stream << poly_mode;
		stream << samples;
	}

	void shader_desc_t::deserialize(istream_t& stream)
	{
		u16 sh_flags	= 0;
		u32 layout_size = 0;

		stream >> debug_name;
		stream >> vertex_entry;
		stream >> pixel_entry;
		stream >> compute_entry;
		stream >> sh_flags;

		u16 att_count = 0;
		stream >> att_count;
		attachments.resize(att_count);
		for (u16 i = 0; i < att_count; i++)
		{
			shader_color_attachment_t& att	 = attachments[i];
			u8						   flags = 0;
			stream >> att.format;
			stream >> att.blend_attachment.blend_enabled;
			stream >> att.blend_attachment.src_alpha_blend_factor;
			stream >> att.blend_attachment.dst_alpha_blend_factor;
			stream >> att.blend_attachment.color_blend_op;
			stream >> att.blend_attachment.src_alpha_blend_factor;
			stream >> att.blend_attachment.dst_alpha_blend_factor;
			stream >> att.blend_attachment.alpha_blend_op;
			stream >> flags;
			att.blend_attachment.color_comp_flags = flags;
		}

		// depth stencil
		{
			u8 flags = 0;
			stream >> depth_stencil_desc.attachment_format;
			stream >> depth_stencil_desc.depth_compare;
			stream >> depth_stencil_desc.back_stencil_state.compare_op;
			stream >> depth_stencil_desc.back_stencil_state.depth_fail_op;
			stream >> depth_stencil_desc.back_stencil_state.fail_op;
			stream >> depth_stencil_desc.back_stencil_state.pass_op;
			stream >> depth_stencil_desc.front_stencil_state.compare_op;
			stream >> depth_stencil_desc.front_stencil_state.depth_fail_op;
			stream >> depth_stencil_desc.front_stencil_state.fail_op;
			stream >> depth_stencil_desc.front_stencil_state.pass_op;
			stream >> depth_stencil_desc.stencil_compare_mask;
			stream >> depth_stencil_desc.stencil_write_mask;
			stream >> flags;
			depth_stencil_desc.flags = flags;
			stream >> depth_bias_clamp;
			stream >> depth_bias_constant;
			stream >> depth_bias_slope;
		}

		u16 inp_count = 0;
		stream >> inp_count;
		inputs.resize(inp_count);
		for (u16 i = 0; i < inp_count; i++)
		{
			vertex_input_t& inp	   = inputs[i];
			u32				offset = 0;
			u32				size   = 0;

			stream >> inp.name;
			stream >> inp.location;
			stream >> inp.index;
			stream >> offset;
			stream >> size;
			stream >> inp.format;

			inp.offset = static_cast<size_t>(offset);
			inp.size   = static_cast<size_t>(size);
		}

		stream >> fill;
		stream >> blend_logic_op;
		stream >> topo;
		stream >> cull;
		stream >> front;
		stream >> poly_mode;
		stream >> samples;
	}

	void compile_variant_t::destroy()
	{
		for (shader_blob_t& b : blobs)
		{
			if (b.data.size != 0)
				delete[] b.data.data;
		}

		blobs.clear();
	}

	void compile_variant_t::serialize(ostream_t& stream, bool address_only) const
	{
		const u32 sz = static_cast<u32>(blobs.size());
		stream << sz;

		for (const shader_blob_t& b : blobs)
		{
			const u32 blob_sz = static_cast<u32>(b.data.size);
			stream << blob_sz;
			stream << b.stage;

			if (b.data.size != 0)
			{
				if (address_only)
				{
					const u64 addr = reinterpret_cast<u64>(b.data.data);
					stream << addr;
				}
				else
				{
					stream.write_raw(b.data.data, b.data.size);
				}
			}
		}
	}

	void compile_variant_t::deserialize(istream_t& stream, bool address_only)
	{
		u32 sz = 0;
		stream >> sz;
		blobs.resize(sz);

		for (u32 i = 0; i < sz; i++)
		{
			shader_blob_t& b	   = blobs[i];
			u32			   blob_sz = 0;
			stream >> blob_sz;
			stream >> b.stage;
			b.data.size = static_cast<size_t>(blob_sz);

			if (blob_sz > 0)
			{
				if (address_only)
				{
					u64 addr = 0;
					stream >> addr;
					b.data.data = reinterpret_cast<u8*>(addr);
				}
				else
				{
					b.data.data = new u8[b.data.size];
					stream.read_to_raw(b.data.data, b.data.size);
				}
			}
		}
	}

	void pso_variant_t::serialize(ostream_t& stream) const
	{
		stream << desc;
		stream << compile_variant_t;
		stream << variant_flags.value();
	}

	void pso_variant_t::deserialize(istream_t& stream)
	{
		u32 flags_val = 0;
		stream >> desc;
		stream >> compile_variant_t;
		stream >> flags_val;
		variant_flags = flags_val;
	}
}
