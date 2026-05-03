// Copyright (c) 2025 Inan Evin

#include "editor_resources.hpp"
#include "editor_directories.hpp"

#include <sfg/data/vector.hpp>
#include <sfg/gfx/backend/backend.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	namespace
	{
		shader_desc_t make_ui_shader_desc(const char* name, bool blend)
		{
			shader_desc_t d						   = {};
			d.vertex_entry						   = "VSMain";
			d.pixel_entry						   = "PSMain";
			d.topo								   = topology::triangle_list;
			d.cull								   = cull_mode::none;
			d.fill								   = fill_mode::solid;
			d.front								   = front_face::cw;
			d.poly_mode							   = polygon_mode::fill;
			d.samples							   = 1;
			d.debug_name						   = name;
			d.depth_stencil_desc.flags			   = 0;
			d.depth_stencil_desc.attachment_format = format_e::undefined;

			d.inputs.push_back({.name = "POSITION", .location = 0, .index = 0, .offset = 0, .size = sizeof(f32) * 2, .format = format_e::r32g32_sfloat});
			d.inputs.push_back({.name = "TEXCOORD", .location = 0, .index = 0, .offset = sizeof(f32) * 2, .size = sizeof(f32) * 2, .format = format_e::r32g32_sfloat});
			d.inputs.push_back({.name = "COLOR", .location = 0, .index = 0, .offset = sizeof(f32) * 4, .size = sizeof(f32) * 4, .format = format_e::r32g32b32a32_sfloat});

			shader_color_attachment_t att				= {};
			att.format									= format_e::b8g8r8a8_srgb;
			att.blend_attachment.blend_enabled			= blend;
			att.blend_attachment.src_color_blend_factor = blend_factor::src_alpha;
			att.blend_attachment.dst_color_blend_factor = blend_factor::one_minus_src_alpha;
			att.blend_attachment.color_blend_op			= blend_op::add;
			att.blend_attachment.src_alpha_blend_factor = blend_factor::one;
			att.blend_attachment.dst_alpha_blend_factor = blend_factor::one_minus_src_alpha;
			att.blend_attachment.alpha_blend_op			= blend_op::add;
			d.attachments.push_back(att);
			return d;
		}
	}

	bool editor_resources_t::init()
	{
		_allocator.init(64 * 1024, alignof(std::max_align_t));
		_resources.reserve(16);

		// const shader_desc_t default_desc = make_ui_shader_desc("ui_default", true);
		// const shader_desc_t text_desc	 = make_ui_shader_desc("ui_text", true);
		// const shader_desc_t sdf_desc	 = make_ui_shader_desc("ui_sdf", true);
		//
		// const string_t assets			 = editor_directories_t::get_editor_assets();
		// const string_t default_path		 = assets + "shaders/ui_default.hlsl";
		// const string_t text_path		 = assets + "shaders/ui_text.hlsl";
		// const string_t sdf_path			 = assets + "shaders/ui_sdf.hlsl";
		// const string_t default_font_path = assets + "fonts/RobotoRegular.ttf";
		//
		// if (load_shader(default_path.c_str(), default_desc) == nullptr)
		// 	return false;
		// if (load_shader(text_path.c_str(), text_desc) == nullptr)
		// 	return false;
		// if (load_shader(sdf_path.c_str(), sdf_desc) == nullptr)
		// 	return false;
		return true;
	}

	void editor_resources_t::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		for (auto& kv : _resources)
		{
			entry_t& entry = kv.second;
			switch (entry.type)
			{
			case editor_resource_type_e::shader: {
				editor_shader_t* sh = static_cast<editor_shader_t*>(entry.ptr);
				if (!sh->handle.is_null())
					backend->destroy_shader(sh->handle);
				break;
			}
			case editor_resource_type_e::texture: {
				editor_texture_t* tx = static_cast<editor_texture_t*>(entry.ptr);
				if (!tx->handle.is_null())
					backend->destroy_texture(tx->handle);
				break;
			}
			}
		}

		_resources.clear();
		_allocator.uninit();
	}

	editor_shader_t* editor_resources_t::load_shader(const char* path, const shader_desc_t& desc)
	{
		gfx_backend* backend = gfx_backend::get();

		if (!file_system::exists(path))
		{
			SFG_ERR("editor_resources: shader path does not exist: {0}", path);
			return nullptr;
		}

		const string_t source = file_system::read_file_as_string(path);
		if (source.empty())
		{
			SFG_ERR("editor_resources: shader file is empty: {0}", path);
			return nullptr;
		}

		const vector_t<string_t> defines		 = {};
		const vector_t<string_t> include_paths	 = {};
		span_t<u8>				 vs_blob		 = {};
		span_t<u8>				 ps_blob		 = {};
		span_t<u8>				 signature_blob	 = {};
		span_t<u8>				 dummy_signature = {};

		if (!backend->compile_shader_vertex_pixel(static_cast<u8>(shader_stage::vertex), source, defines, include_paths, "VSMain", vs_blob, true, signature_blob))
		{
			SFG_ERR("editor_resources: vertex compile failed for {0}", path);
			return nullptr;
		}

		if (!backend->compile_shader_vertex_pixel(static_cast<u8>(shader_stage::fragment), source, defines, include_paths, "PSMain", ps_blob, false, dummy_signature))
		{
			SFG_ERR("editor_resources: pixel compile failed for {0}", path);
			delete[] vs_blob.data;
			delete[] signature_blob.data;
			return nullptr;
		}

		vector_t<shader_blob_t> blobs;
		blobs.push_back({.stage = shader_stage::vertex, .data = vs_blob});
		blobs.push_back({.stage = shader_stage::fragment, .data = ps_blob});

		const gfx_shader_handle handle = backend->create_shader(desc, blobs, {}, signature_blob);

		delete[] vs_blob.data;
		delete[] ps_blob.data;
		delete[] signature_blob.data;

		if (handle.is_null())
		{
			SFG_ERR("editor_resources: create_shader failed for {0}", path);
			return nullptr;
		}

		editor_shader_t* shader = static_cast<editor_shader_t*>(_allocator.allocate(sizeof(editor_shader_t), alignof(editor_shader_t)));
		new (shader) editor_shader_t{};
		shader->handle = handle;

		_resources[TO_SID(path)] = entry_t{
			.ptr  = shader,
			.type = editor_resource_type_e::shader,
		};

		SFG_TRACE("loaded: {0}", path);
		return shader;
	}

	editor_texture_t* editor_resources_t::load_texture(const char* path)
	{
		gfx_backend* backend = gfx_backend::get();

		if (!file_system::exists(path))
		{
			SFG_ERR("editor_resources: texture path does not exist: {0}", path);
			return nullptr;
		}

		texture_desc_t desc = {};
		desc.texture_format = format_e::r8g8b8a8_srgb;
		desc.flags			= texture_flags::tf_sampled | texture_flags::tf_transfer_dest | texture_flags::tf_is_2d;
		desc.mip_levels		= 1;
		desc.array_length	= 1;
		desc.samples		= 1;
		desc.debug_name		= path;

		// vec2u16_t pixel_size  = {};
		// u8        channels    = 0;
		// void*     pixels      = image_util_t::load_from_file(path, pixel_size, channels);
		// desc.size             = pixel_size;
		// const gfx_texture_handle handle = backend->create_texture(desc);
		//
		// resource_desc_t staging_desc = {};
		// staging_desc.size            = static_cast<u32>(pixel_size.x) * pixel_size.y * 4;
		// staging_desc.flags           = resource_flags::rf_cpu_visible;
		// staging_desc.debug_name      = "editor_texture_staging";
		// const gfx_resource_handle staging = backend->create_resource(staging_desc);
		//
		// editor_resources_t::queue_texture_upload(handle, staging, pixels, pixel_size, channels);
		// image_util_t::free(pixels);

		const gfx_texture_handle handle = backend->create_texture(desc);
		if (handle.is_null())
		{
			SFG_ERR("editor_resources: create_texture failed for {0}", path);
			return nullptr;
		}

		editor_texture_t* texture = static_cast<editor_texture_t*>(_allocator.allocate(sizeof(editor_texture_t), alignof(editor_texture_t)));
		new (texture) editor_texture_t{};
		texture->handle = handle;

		_resources[TO_SID(path)] = entry_t{
			.ptr  = texture,
			.type = editor_resource_type_e::texture,
		};
		SFG_TRACE("loaded: {0}", path);
		return texture;
	}

}
