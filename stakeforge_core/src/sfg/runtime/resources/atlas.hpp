// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	struct font_data_t;

	class atlas_t
	{
	public:
		atlas_t()						   = default;
		atlas_t(const atlas_t&)			   = delete;
		atlas_t& operator=(const atlas_t&) = delete;
		~atlas_t();

		void init(u32 width, u32 height, bool is_lcd);
		void uninit();

		bool add_font(font_data_t* font);
		void remove_font(font_data_t* font);

		inline bool is_empty() const
		{
			return _font_count == 0;
		}

		inline u32 get_width() const
		{
			return _width;
		}
		inline u32 get_height() const
		{
			return _height;
		}
		inline u8* get_data() const
		{
			return _data;
		}
		inline u32 get_data_size() const
		{
			return _data_size;
		}
		inline bool get_is_lcd() const
		{
			return _is_lcd;
		}
		inline u32 get_id() const
		{
			return _id;
		}
		inline void set_id(u32 i)
		{
			_id = i;
		}
		inline bool is_dirty() const
		{
			return _dirty;
		}
		inline void mark_dirty()
		{
			_dirty = true;
		}
		inline void clear_dirty()
		{
			_dirty = false;
		}

	private:
		u8*	 _data		   = nullptr;
		u32	 _data_size	   = 0;
		u32	 _width		   = 0;
		u32	 _height	   = 0;
		u32	 _id		   = 0xFFFFFFFFu;
		u32	 _vertical_pos = 0;
		u32	 _font_count   = 0;
		bool _is_lcd	   = false;
		bool _dirty		   = false;
	};
}
