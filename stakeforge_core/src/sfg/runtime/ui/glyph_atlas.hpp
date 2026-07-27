// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	struct font_runtime_t;
}

namespace sfg::ui
{
	struct glyph_entry_t
	{
		f32 uv_x			= 0.0f;
		f32 uv_y			= 0.0f;
		f32 uv_w			= 0.0f;
		f32 uv_h			= 0.0f;
		i16 atlas_x			= 0;
		i16 atlas_y			= 0;
		i16 width			= 0;
		i16 height			= 0;
		f32 left_bearing	= 0.0f;
		f32 top_bearing		= 0.0f;
		f32 advance_x		= 0.0f;
		u32 last_used_frame = 0;
	};

	struct size_metrics_t
	{
		f32 ascent_px	   = 0.0f;
		f32 descent_px	   = 0.0f;
		f32 line_height_px = 0.0f;
		f32 px_height	   = 0.0f;
	};

	struct glyph_atlas_config_t
	{
		u32 width							= 4096;
		u32 height							= 4096;
		u32 staging_budget_bytes			= 4u << 20; // 4 MB per back buffer slot
		u32 glyph_initial_capacity			= 1024;
		u32 size_metric_initial_capacity	= 64;
		u32 shelf_initial_capacity			= 64;
		u32 pending_upload_initial_capacity = 256;
	};

	class glyph_atlas_t final
	{
	public:
		glyph_atlas_t() = default;
		~glyph_atlas_t();
		glyph_atlas_t(const glyph_atlas_t&)			   = delete;
		glyph_atlas_t& operator=(const glyph_atlas_t&) = delete;

		void init(const glyph_atlas_config_t& cfg = {});
		void uninit();

		const glyph_entry_t* request_glyph(const font_runtime_t* font, u32 codepoint, u32 px_size, glyph_raster_mode_e mode);
		size_metrics_t		 request_size_metrics(const font_runtime_t* font, u32 px_size);
		f32					 get_kern_advance(const font_runtime_t* font, u32 prev_cp, u32 next_cp, u32 px_size) const;

		void tick(u32 frame_index);
		void drain_uploads(u8 frame_slot);

		render_resource_handle_t get_texture() const;
		u32						 get_width() const;
		u32						 get_height() const;
		bool					 is_initialized() const;

	private:
		struct shelf_t
		{
			i32 y		 = 0;
			i32 height	 = 0;
			i32 x_cursor = 0;
		};

		struct pending_upload_t
		{
			i16 atlas_x = 0;
			i16 atlas_y = 0;
			i16 width	= 0;
			i16 height	= 0;
			u8* rgba	= nullptr;
		};

		struct staging_slot_t
		{
			render_resource_handle_t buffer	  = {};
			u32						 capacity = 0;
		};

		bool allocate_slot(i32 w, i32 h, i16& out_x, i16& out_y);

	private:
		hash_map_t<u64, glyph_entry_t>	_entries;
		hash_map_t<u64, size_metrics_t> _metrics;
		vector_t<shelf_t>				_shelves;
		vector_t<pending_upload_t>		_pending;
		staging_slot_t					_staging[BACK_BUFFER_COUNT] = {};
		render_resource_handle_t		_texture					= {};
		u32								_width						= 0;
		u32								_height						= 0;
		u32								_frame						= 0;
		bool							_transitioned				= false;
	};
}
