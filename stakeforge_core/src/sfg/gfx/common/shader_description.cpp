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
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/memory/memory.hpp>

namespace sfg
{
	namespace
	{
		void set_desc_name(char* dst, size_t capacity, const char* src)
		{
			SFG_ASSERT(src != nullptr);
			if (src == nullptr)
				return;
			const size_t len = std::strlen(src);
			SFG_ASSERT(len < capacity);
			if (len >= capacity)
				return;
			SFG_MEMCPY(dst, src, len + 1);
		}
	}

	// --- stencil_op ---
	// --- load_op ---
	// --- compare_op ---
	// --- store_op ---
	void vertex_input_t::set_name(const char* name)
	{
		set_desc_name(this->name, MAX_NAME, name);
	}

	void shader_desc_t::set_name(const char* name)
	{
		set_desc_name(debug_name, MAX_DEBUG_NAME, name);
	}

	void shader_desc_t::set_vertex_entry(const char* name)
	{
		set_desc_name(vertex_entry, MAX_ENTRY_NAME, name);
	}

	void shader_desc_t::set_pixel_entry(const char* name)
	{
		set_desc_name(pixel_entry, MAX_ENTRY_NAME, name);
	}

	void shader_desc_t::set_compute_entry(const char* name)
	{
		set_desc_name(compute_entry, MAX_ENTRY_NAME, name);
	}

	void shader_desc_t::add_attachment(const shader_color_attachment_t& attachment)
	{
		SFG_ASSERT(attachment_count < MAX_ATTACHMENTS);
		if (attachment_count >= MAX_ATTACHMENTS)
			return;
		attachments[attachment_count++] = attachment;
	}

	void shader_desc_t::add_input(const vertex_input_t& input)
	{
		SFG_ASSERT(input_count < MAX_VERTEX_INPUTS);
		if (input_count >= MAX_VERTEX_INPUTS)
			return;
		inputs[input_count++] = input;
	}

	void shader_desc_t::serialize(ostream_t& stream) const
	{
		stream << string_t(debug_name);
		stream << string_t(vertex_entry);
		stream << string_t(pixel_entry);
		stream << string_t(compute_entry);
		stream << flags.value();

		const u16 att_count = attachment_count;
		stream << att_count;

		for (u8 i = 0; i < attachment_count; ++i)
		{
			const shader_color_attachment_t& att = attachments[i];
			stream << att.format;
			stream << att.blend_attachment.blend_enabled;
			stream << att.blend_attachment.src_color_blend_factor;
			stream << att.blend_attachment.dst_color_blend_factor;
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

		const u16 inp_count = input_count;
		stream << inp_count;

		for (u8 i = 0; i < input_count; ++i)
		{
			const vertex_input_t& inp = inputs[i];
			stream << string_t(inp.name);
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
		u16 sh_flags = 0;

		string_t name;
		string_t vs;
		string_t ps;
		string_t cs;
		stream >> name;
		stream >> vs;
		stream >> ps;
		stream >> cs;
		set_name(name.c_str());
		set_vertex_entry(vs.c_str());
		set_pixel_entry(ps.c_str());
		set_compute_entry(cs.c_str());
		stream >> sh_flags;
		flags = sh_flags;

		u16 att_count = 0;
		stream >> att_count;
		SFG_ASSERT(att_count <= MAX_ATTACHMENTS);
		if (att_count > MAX_ATTACHMENTS)
			return;
		attachment_count = static_cast<u8>(att_count);
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
		SFG_ASSERT(inp_count <= MAX_VERTEX_INPUTS);
		if (inp_count > MAX_VERTEX_INPUTS)
			return;
		input_count = static_cast<u8>(inp_count);
		for (u16 i = 0; i < inp_count; i++)
		{
			vertex_input_t& inp	   = inputs[i];
			u32				offset = 0;
			u32				size   = 0;
			string_t		name;

			stream >> name;
			inp.set_name(name.c_str());
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

namespace sfg
{
	vertex_input_reflection_t::vertex_input_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "vertex_input_t",
			.display_name = "Vertex Input",
			.fields =
				{
					{
						.name		  = "name",
						.display_name = "Name",
						.offset		  = offsetof(vertex_input_t, name),
						.size		  = sizeof(vertex_input_t::name),
						.type		  = reflected_value_type_e_v2::char_array,
					},
					{
						.name		  = "offset",
						.display_name = "Offset",
						.offset		  = offsetof(vertex_input_t, offset),
						.size		  = sizeof(size_t),
						.type		  = reflected_value_type_e_v2::u64,
					},
					{
						.name		  = "size",
						.display_name = "Size",
						.offset		  = offsetof(vertex_input_t, size),
						.size		  = sizeof(size_t),
						.type		  = reflected_value_type_e_v2::u64,
					},
					{
						.name		  = "format",
						.display_name = "Format",
						.sub_type_id  = type_id_t<format_e>::value,
						.offset		  = offsetof(vertex_input_t, format),
						.size		  = sizeof(format_e),
						.type		  = reflected_value_type_e_v2::u8,
					},
					{
						.name		  = "location",
						.display_name = "Location",
						.offset		  = offsetof(vertex_input_t, location),
						.size		  = sizeof(u8),
						.type		  = reflected_value_type_e_v2::u8,
					},
					{
						.name		  = "index",
						.display_name = "Index",
						.offset		  = offsetof(vertex_input_t, index),
						.size		  = sizeof(u8),
						.type		  = reflected_value_type_e_v2::u8,
					},
				},
			.type_id   = type_id_t<vertex_input_t>::value,
			.size	   = sizeof(vertex_input_t),
			.alignment = alignof(vertex_input_t),
		});
	}

	cull_mode_reflection_t::cull_mode_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "cull_mode",
			.display_name = "Cull Mode",
			.fields =
				{
					{.name = "none", .display_name = "None"},
					{.name = "front", .display_name = "Front"},
					{.name = "back", .display_name = "Back"},
				},
			.type_id   = type_id_t<cull_mode>::value,
			.size	   = sizeof(cull_mode),
			.alignment = alignof(cull_mode),
			.flags	   = reflected_type_flag_enum,
		});
	}

	fill_mode_reflection_t::fill_mode_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "fill_mode",
			.display_name = "Fill Mode",
			.fields =
				{
					{.name = "solid", .display_name = "Solid"},
					{.name = "wireframe", .display_name = "Wireframe"},
				},
			.type_id   = type_id_t<fill_mode>::value,
			.size	   = sizeof(fill_mode),
			.alignment = alignof(fill_mode),
			.flags	   = reflected_type_flag_enum,
		});
	}

	front_face_reflection_t::front_face_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "front_face",
			.display_name = "Front Face",
			.fields =
				{
					{.name = "ccw", .display_name = "Counter Clockwise"},
					{.name = "cw", .display_name = "Clockwise"},
				},
			.type_id   = type_id_t<front_face>::value,
			.size	   = sizeof(front_face),
			.alignment = alignof(front_face),
			.flags	   = reflected_type_flag_enum,
		});
	}

	blend_factor_reflection_t::blend_factor_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "blend_factor",
			.display_name = "Blend Factor",
			.fields =
				{
					{.name = "zero", .display_name = "Zero"},
					{.name = "one", .display_name = "One"},
					{.name = "src_color", .display_name = "Src Color"},
					{.name = "one_minus_src_color", .display_name = "One Minus Src Color"},
					{.name = "dst_color", .display_name = "Dst Color"},
					{.name = "one_minus_dst_color", .display_name = "One Minus Dst Color"},
					{.name = "src_alpha", .display_name = "Src Alpha"},
					{.name = "one_minus_src_alpha", .display_name = "One Minus Src Alpha"},
					{.name = "dst_alpha", .display_name = "Dst Alpha"},
					{.name = "one_minus_dst_alpha", .display_name = "One Minus Dst Alpha"},
				},
			.type_id   = type_id_t<blend_factor>::value,
			.size	   = sizeof(blend_factor),
			.alignment = alignof(blend_factor),
			.flags	   = reflected_type_flag_enum,
		});
	}

	blend_op_reflection_t::blend_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "blend_op",
			.display_name = "Blend Op",
			.fields =
				{
					{.name = "add", .display_name = "Add"},
					{.name = "subtract", .display_name = "Subtract"},
					{.name = "reverse_subtract", .display_name = "Reverse Subtract"},
					{.name = "min", .display_name = "Min"},
					{.name = "max", .display_name = "Max"},
				},
			.type_id   = type_id_t<blend_op>::value,
			.size	   = sizeof(blend_op),
			.alignment = alignof(blend_op),
			.flags	   = reflected_type_flag_enum,
		});
	}

	stencil_op_reflection_t::stencil_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "stencil_op",
			.display_name = "Stencil Op",
			.fields =
				{
					{.name = "keep", .display_name = "Keep"},
					{.name = "zero", .display_name = "Zero"},
					{.name = "replace", .display_name = "Replace"},
					{.name = "increment_clamp", .display_name = "Increment Clamp"},
					{.name = "decrement_clamp", .display_name = "Decrement Clamp"},
					{.name = "invert", .display_name = "Invert"},
					{.name = "increment_wrap", .display_name = "Increment Wrap"},
					{.name = "decrement_wrap", .display_name = "Decrement Wrap"},
				},
			.type_id   = type_id_t<stencil_op>::value,
			.size	   = sizeof(stencil_op),
			.alignment = alignof(stencil_op),
			.flags	   = reflected_type_flag_enum,
		});
	}

	compare_op_reflection_t::compare_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "compare_op",
			.display_name = "Compare Op",
			.fields =
				{
					{.name = "never", .display_name = "Never"},
					{.name = "less", .display_name = "Less"},
					{.name = "equal", .display_name = "Equal"},
					{.name = "lequal", .display_name = "Less Equal"},
					{.name = "greater", .display_name = "Greater"},
					{.name = "nequal", .display_name = "Not Equal"},
					{.name = "gequal", .display_name = "Greater Equal"},
					{.name = "always", .display_name = "Always"},
				},
			.type_id   = type_id_t<compare_op>::value,
			.size	   = sizeof(compare_op),
			.alignment = alignof(compare_op),
			.flags	   = reflected_type_flag_enum,
		});
	}

	store_op_reflection_t::store_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "store_op",
			.display_name = "Store Op",
			.fields =
				{
					{.name = "store", .display_name = "Store"},
					{.name = "dont_care", .display_name = "Dont Care"},
					{.name = "none", .display_name = "None"},
				},
			.type_id   = type_id_t<store_op>::value,
			.size	   = sizeof(store_op),
			.alignment = alignof(store_op),
			.flags	   = reflected_type_flag_enum,
		});
	}

	load_op_reflection_t::load_op_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "load_op",
			.display_name = "Load Op",
			.fields =
				{
					{.name = "load", .display_name = "Load"},
					{.name = "clear", .display_name = "Clear"},
					{.name = "dont_care", .display_name = "Dont Care"},
					{.name = "none", .display_name = "None"},
				},
			.type_id   = type_id_t<load_op>::value,
			.size	   = sizeof(load_op),
			.alignment = alignof(load_op),
			.flags	   = reflected_type_flag_enum,
		});
	}

	stencil_state_reflection_t::stencil_state_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "stencil_state_t",
			.display_name = "Stencil State",
			.fields =
				{
					{
						.name		  = "fail_op",
						.display_name = "Fail Op",
						.sub_type_id  = type_id_t<stencil_op>::value,
						.offset		  = offsetof(stencil_state_t, fail_op),
						.size		  = sizeof(stencil_op),
						.type		  = reflected_value_type_e_v2::u8,
					},
					{
						.name		  = "pass_op",
						.display_name = "Pass Op",
						.sub_type_id  = type_id_t<stencil_op>::value,
						.offset		  = offsetof(stencil_state_t, pass_op),
						.size		  = sizeof(stencil_op),
						.type		  = reflected_value_type_e_v2::u8,
					},
					{
						.name		  = "depth_fail_op",
						.display_name = "Depth Fail Op",
						.sub_type_id  = type_id_t<stencil_op>::value,
						.offset		  = offsetof(stencil_state_t, depth_fail_op),
						.size		  = sizeof(stencil_op),
						.type		  = reflected_value_type_e_v2::u8,
					},
					{
						.name		  = "compare_op",
						.display_name = "Compare Op",
						.sub_type_id  = type_id_t<compare_op>::value,
						.offset		  = offsetof(stencil_state_t, compare_op),
						.size		  = sizeof(compare_op),
						.type		  = reflected_value_type_e_v2::u8,
					},
				},
			.type_id   = type_id_t<stencil_state_t>::value,
			.size	   = sizeof(stencil_state_t),
			.alignment = alignof(stencil_state_t),
		});
	}
}
