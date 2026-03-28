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

#include "vekt_defines.hpp"
#include "math/vector2.hpp"
#include "math/vector4.hpp"

#include <memory>
#include <assert.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <utility> // For std::swap, std::move
#define VEKT_INLINE inline
#define VEKT_API	extern

#undef min
#undef max

namespace vekt
{
	////////////////////////////////////////////////////////////////////////////////
	// :: COMMON DEFINES
	////////////////////////////////////////////////////////////////////////////////

#define M_PI	  3.14159265358979f
#define DEG_2_RAD 0.0174533f
#ifndef VEKT_USER_DATA_SIZE
#define VEKT_USER_DATA_SIZE 1024
#endif

#if defined _MSC_VER && !__INTEL_COMPILER
#define ALIGNED_MALLOC(SZ, ALIGN) _aligned_malloc(SZ, ALIGN)
#define ALIGNED_FREE(PTR)		  _aligned_free(PTR);
#else
#include <stdlib.h>
#define ALIGNED_MALLOC(SZ, ALIGN) std::aligned_alloc(ALIGN, SZ)
#define ALIGNED_FREE(PTR)		  std::free(PTR)
#endif

#define MEMMOVE(...) memmove(__VA_ARGS__) ALIGNED_MALLOC
#define REALLOC(...) realloc(__VA_ARGS__)
#define MEMCPY(...)	 memcpy(__VA_ARGS__)
#define ASSERT(...)	 assert(__VA_ARGS__)
#define MEMSET(...)	 memset(__VA_ARGS__)

#define VEKT_STRING_CSTR
	// #include <string>
#define VEKT_STRING const char*
	// #define VEKT_STRING std::string

	////////////////////////////////////////////////////////////////////////////////
	// :: LOGS & CONFIGS
	////////////////////////////////////////////////////////////////////////////////

	enum class log_verbosity
	{
		info,
		warning,
		error
	};

	typedef void (*log_callback)(log_verbosity, const char*, ...);
	struct config_data
	{
		log_callback on_log		  = nullptr;
		unsigned int atlas_width  = 1024;
		unsigned int atlas_height = 1024;
	};

	extern config_data config;

#define V_LOG(...)                                                                                                                                                                                                                                                 \
	if (vekt::config.on_log)                                                                                                                                                                                                                                       \
	vekt::config.on_log(vekt::log_verbosity::info, __VA_ARGS__)
#define V_ERR(...)                                                                                                                                                                                                                                                 \
	if (vekt::config.on_log)                                                                                                                                                                                                                                       \
	vekt::config.on_log(vekt::log_verbosity::error, __VA_ARGS__)
#define V_WARN(...)                                                                                                                                                                                                                                                \
	if (vekt::config.on_log)                                                                                                                                                                                                                                       \
	vekt::config.on_log(vekt::log_verbosity::warning, __VA_ARGS__)

#define MALLOC(SZ) malloc(SZ)
#define FREE(X)	   free(X)

	////////////////////////////////////////////////////////////////////////////////
	// :: COMMON CONTAINERS
	////////////////////////////////////////////////////////////////////////////////

	template <typename T> class vector
	{
	public:
		using iterator								   = T*;
		using const_iterator						   = const T*;
		static constexpr unsigned int initial_capacity = 4;

		vector(){};
		vector(const vector<T>& other)
		{
			if (other.empty())
			{
				return;
			}

			_capacity = other._capacity;
			_elements = reinterpret_cast<T*>(MALLOC(_capacity * sizeof(T)));
			if (!_elements)
				throw std::bad_alloc();

			for (unsigned int i = 0; i < other._count; ++i)
			{
				new (&_elements[i]) T(other._elements[i]);
			}
			_count = other._count;
		}

		vector<T>& operator=(const vector<T>& other)
		{
			if (this == &other)
			{
				return *this;
			}

			clear();

			if (other.empty())
			{
				return *this;
			}

			_capacity = other._capacity;
			_elements = reinterpret_cast<T*>(MALLOC(_capacity * sizeof(T)));
			if (!_elements)
			{
				throw std::bad_alloc();
			}

			_count = other._count;
			MEMCPY(_elements, other._elements, _count * sizeof(T));

			return *this;
		}

		~vector()
		{
			clear();
		}

		inline void push_back(const T& elem)
		{
			check_grow();
			new (&_elements[_count]) T(elem);
			_count++;
		}

		inline void push_back(T&& elem)
		{
			check_grow();
			new (&_elements[_count]) T(std::move(elem));
			_count++;
		}

		inline void pop_back()
		{
			remove_index(_count - 1);
		}

		inline void increment_back()
		{
			const bool req_place = _count >= _capacity;
			check_grow();
			if (req_place)
				new (&_elements[_count]) T();
			_count++;
		}

		inline T& get_back()
		{
			return _elements[_count - 1];
		}

		inline void remove_index(unsigned int index)
		{
			if (index >= _count)
			{
				return;
			}
			_elements[index].~T();

			if (index < _count - 1)
			{
				for (unsigned int i = index; i < _count - 1; ++i)
					_elements[i] = std::move(_elements[i + 1]);
			}
			_count--;
		}

		inline void remove(T& elem)
		{
			for (unsigned int i = 0; i < _count; ++i)
			{
				if (_elements[i] == elem)
				{
					remove_index(i);
					return;
				}
			}
		}

		inline void remove(iterator it)
		{
			remove(*it);
		}

		inline void clear()
		{
			for (unsigned int i = 0; i < _count; ++i)
				_elements[i].~T();

			if (_elements)
			{
				FREE(_elements);
			}
			_elements = nullptr;
			_count	  = 0;
			_capacity = 0;
		}

		inline void reserve(unsigned int new_capacity)
		{
			if (new_capacity <= _capacity)
			{
				return;
			}

			T* new_elements = (T*)MALLOC(new_capacity * sizeof(T));
			if (!new_elements)
			{
				throw std::bad_alloc();
			}

			for (unsigned int i = 0; i < _count; ++i)
			{
				new (&new_elements[i]) T(std::move(_elements[i]));
				_elements[i].~T();
			}

			for (unsigned int i = _capacity; i < new_capacity; i++)
			{
				new (&new_elements[i]) T();
			}

			if (_elements)
			{
				FREE(_elements);
			}

			_elements = new_elements;
			_capacity = new_capacity;
		}

		inline void resize(unsigned int sz, bool call_dest = true)
		{
			// Handle destructors if shrinking.
			if (call_dest && sz < _count)
			{
				for (unsigned int i = sz; i < _count; ++i)
					_elements[i].~T();
			}

			if (sz > _capacity)
			{
				T* new_elements = (T*)MALLOC(sz * sizeof(T));
				if (!new_elements)
					throw std::bad_alloc();

				// Move existing elements
				for (unsigned int i = 0; i < _count; ++i)
				{
					new (&new_elements[i]) T(std::move(_elements[i]));
					_elements[i].~T();
				}

				FREE(_elements);
				_elements = new_elements;
				_capacity = sz;
			}

			// If growing, the new elements are default-constructed
			if (sz > _count)
			{
				for (unsigned int i = _count; i < sz; ++i)
					new (&_elements[i]) T();
			}
			_count = sz;
		}

		inline void resize_explicit(unsigned int sz)
		{
			if (sz > _capacity)
			{
				T* new_elements = (T*)MALLOC(sz * sizeof(T));
				if (!new_elements)
					throw std::bad_alloc();

				// Move existing elements
				for (unsigned int i = 0; i < _count; ++i)
				{
					new (&new_elements[i]) T(std::move(_elements[i]));
					_elements[i].~T();
				}

				FREE(_elements);
				_elements = new_elements;
				_capacity = sz;
			}

			_count = sz;
		}

		inline void reverse()
		{
			std::reverse(begin(), end());
		}

		inline int index_of(const T& t) const
		{
			for (int i = 0; i < _count; i++)
				if (t == _elements[i])
					return i;
			return -1;
		};

		inline iterator find(T& t)
		{
			for (unsigned int i = 0; i < _count; i++)
			{
				if (t == _elements[i])
					return _elements + i;
			}
			return end();
		};

		inline iterator find(std::function<bool(const T&)> predicate)
		{
			for (unsigned int i = 0; i < _count; i++)
			{
				if (predicate(_elements[i]))
					return _elements + i;
			}
			return end();
		};

		inline T* data() const
		{
			return _elements;
		}

		T& operator[](unsigned int index)
		{
			return _elements[index];
		}
		const T& operator[](unsigned int index) const
		{
			return _elements[index];
		}
		inline bool empty() const
		{
			return _count == 0;
		}
		inline unsigned int size() const
		{
			return _count;
		}

		inline iterator begin()
		{
			return _elements;
		}
		inline const_iterator begin() const
		{
			return _elements;
		}
		inline const_iterator cbegin() const
		{
			return _elements;
		}
		inline iterator end()
		{
			return _elements + _count;
		}
		inline const_iterator end() const
		{
			return _elements + _count;
		}
		inline const_iterator cend() const
		{
			return _elements + _count;
		}

	private:
		inline void check_grow()
		{
			if (_count >= _capacity)
			{
				unsigned int new_capacity = (_capacity == 0) ? initial_capacity : _capacity * 2;
				reserve(new_capacity);
			}
		}

	private:
		T*			 _elements = nullptr;
		unsigned int _count	   = 0;
		unsigned int _capacity = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	// :: VECTORS & MATH
	////////////////////////////////////////////////////////////////////////////////

	class math
	{
	public:
		template <typename T> static inline T max(T a, T b)
		{
			return a > b ? a : b;
		}
		template <typename T> static inline T min(T a, T b)
		{
			return a < b ? a : b;
		}
		static inline f32 equals(f32 a, f32 b, f32 eps = 0.0001f)
		{
			return a > b - eps && a < b + eps;
		}
		static inline f32 cos(f32 x)
		{
			return std::cos(x);
		}
		static inline f32 sin(f32 x)
		{
			return std::sin(x);
		}
		static inline f32 lerp(f32 a, f32 b, f32 t)
		{
			return a + (b - a) * t;
		}
		static inline f32 ceilf(f32 f)
		{
			return std::ceilf(f);
		}
		static inline f32 remap(f32 val, f32 from_low, f32 from_high, f32 to_low, f32 to_high)
		{
			return to_low + (val - from_low) * (to_high - to_low) / (from_high - from_low);
		}
		static inline f32 clamp(f32 f, f32 min, f32 max)
		{
			if (f > max)
				f = max;
			else if (f < min)
				f = min;
			return f;
		}
	};

#define VEKT_VEC2 SFG::vector2

#if 0

	struct vec2
	{
		f32 x = 0.0f;
		f32 y = 0.0f;

		vec2 operator+(const vec2& other) const
		{
			vec2 v = {};
			v.x	   = x + other.x;
			v.y	   = y + other.y;
			return v;
		}

		vec2 operator*(f32 f) const
		{
			vec2 v = *this;
			v.x *= f;
			v.y *= f;
			return v;
		}

		vec2 operator-(const vec2& other) const
		{
			vec2 v = *this;
			v.x -= other.x;
			v.y -= other.y;
			return v;
		}

		vec2 operator/=(const vec2& other)
		{
			x /= other.x;
			y /= other.y;
			return *this;
		}

		vec2 operator/=(f32 f)
		{
			x /= f;
			y /= f;
			return *this;
		}

		vec2 operator*=(f32 f)
		{
			x *= f;
			y *= f;
			return *this;
		}

		vec2 operator+=(const vec2& other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		bool operator==(const vec2& other) const
		{
			return math::equals(x, other.x) && math::equals(y, other.y);
		}

		inline void normalize()
		{
			const f32 s = mag();
			x /= s;
			y /= s;
		}

		inline vec2 normalized()
		{
			vec2 v = *this;
			v.normalize();
			return v;
		}

		inline f32 mag()
		{
			return sqrt(x * x + y * y);
		}
		inline f32 mag2()
		{
			return x * x + y * y;
		}
	};

#endif

#define VEKT_VEC4 SFG::vector4

#if 0

	struct vec4
	{
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;
		f32 w = 0.0f;

		bool equals(const vec4& other, f32 eps = 0.1f) const
		{
			return math::equals(x, other.x, eps) && math::equals(y, other.y, eps) && math::equals(z, other.z, eps) && math::equals(w, other.w, eps);
		}
		bool is_point_inside(f32 _x, f32 _y) const
		{
			return _x >= x && _x <= x + z && _y >= y && _y <= y + w;
		}

		static inline vec4 lerp(const vec4& a, const vec4& b, f32 t)
		{
			return vec4(math::lerp(a.x, b.x, t), math::lerp(a.y, b.y, t), math::lerp(a.z, b.z, t), math::lerp(a.w, b.w, t));
		}

		bool operator==(const vec4& other) const
		{
			return math::equals(x, other.x) && math::equals(y, other.y) && math::equals(z, other.z) && math::equals(w, other.w);
		}

		vec4 operator+(const vec4& other) const
		{
			vec4 v = {};
			v.x	   = x + other.x;
			v.y	   = y + other.y;
			v.z	   = z + other.z;
			v.w	   = w + other.w;
			return v;
		}

		vec4 operator-(const vec4& other) const
		{
			vec4 v = {};
			v.x	   = x - other.x;
			v.y	   = y - other.y;
			v.z	   = z - other.z;
			v.w	   = w - other.w;
			return v;
		}

		vec4 operator*(f32 f) const
		{
			vec4 v = *this;
			v.x *= f;
			v.y *= f;
			v.z *= f;
			v.w *= f;
			return v;
		}
	};

#endif

	////////////////////////////////////////////////////////////////////////////////
	// :: WIDGET UTILS
	////////////////////////////////////////////////////////////////////////////////

	enum size_flags
	{
		sf_x_relative		= 1 << 0,
		sf_y_relative		= 1 << 1,
		sf_x_abs			= 1 << 2,
		sf_y_abs			= 1 << 3,
		sf_x_copy_y			= 1 << 4,
		sf_y_copy_x			= 1 << 5,
		sf_x_total_children = 1 << 6,
		sf_x_max_children	= 1 << 7,
		sf_y_total_children = 1 << 8,
		sf_y_max_children	= 1 << 9,
		sf_x_fill			= 1 << 10,
		sf_y_fill			= 1 << 11,
		sf_custom_pass		= 1 << 12,
	};

	enum pos_flags
	{
		pf_x_relative		= 1 << 0,
		pf_y_relative		= 1 << 1,
		pf_x_abs			= 1 << 2,
		pf_y_abs			= 1 << 3,
		pf_x_anchor_center	= 1 << 4,
		pf_x_anchor_end		= 1 << 5,
		pf_y_anchor_center	= 1 << 6,
		pf_y_anchor_end		= 1 << 7,
		pf_child_pos_row	= 1 << 8,
		pf_child_pos_column = 1 << 9,
		pf_custom_pass		= 1 << 10,
		pf_overlay			= 1 << 11,
	};

	enum class helper_pos_type
	{
		absolute,
		relative,
	};

	enum class helper_size_type
	{
		absolute,
		relative,
		fill,
		max_children,
		total_children,
		copy_other,
	};

	enum class helper_anchor_type
	{
		start,
		center,
		end
	};

	enum class direction
	{
		horizontal,
		vertical
	};

	struct margins
	{
		f32 top	   = 0.0f;
		f32 bottom = 0.0f;
		f32 left   = 0.0f;
		f32 right  = 0.0f;
	};

	////////////////////////////////////////////////////////////////////////////////
	// :: INPUT
	////////////////////////////////////////////////////////////////////////////////

	enum class input_event_type
	{
		pressed,
		released,
		repeated,
	};

	enum class input_event_result
	{
		handled,
		not_handled,
	};

	enum class input_event_phase
	{
		tunneling,
		bubbling,
	};

	struct mouse_event
	{
		input_event_type type	  = input_event_type::pressed;
		int				 button	  = 0;
		VEKT_VEC2		 position = VEKT_VEC2();
	};

	struct mouse_wheel_event
	{
		f32 amount = 0.0f;
	};

	struct key_event
	{
		input_event_type type	   = input_event_type::pressed;
		int				 key	   = 0;
		int				 scan_code = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	// :: WIDGET GFX
	////////////////////////////////////////////////////////////////////////////////

	enum class gfx_type
	{
		none,
		filled_rect,
		stroke_rect,
		text,
	};

	enum gfx_flags
	{
		gfx_is_rect			 = 1 << 0,
		gfx_is_stroke		 = 1 << 1,
		gfx_is_text			 = 1 << 2,
		gfx_is_text_cached	 = 1 << 4,
		gfx_has_stroke		 = 1 << 4,
		gfx_has_aa			 = 1 << 5,
		gfx_has_second_color = 1 << 6,
		gfx_has_rounding	 = 1 << 7,
		gfx_clip_children	 = 1 << 8,
		gfx_invisible		 = 1 << 9,
		gfx_custom_pass		 = 1 << 10,
		gfx_has_hover_color	 = 1 << 11,
		gfx_has_press_color	 = 1 << 12,
		gfx_focusable		 = 1 << 13,
	};

	struct stroke_props
	{
		VEKT_VEC4	 color	   = {};
		unsigned int thickness = 0;
	};

	struct aa_props
	{
		unsigned int thickness = 0;
	};

	struct second_color_props
	{
		VEKT_VEC4 color		= VEKT_VEC4(1, 1, 1, 1);
		direction direction = direction::horizontal;
	};

	struct input_color_props
	{
		VEKT_VEC4 hovered_color	 = VEKT_VEC4(1, 1, 1, 1);
		VEKT_VEC4 pressed_color	 = VEKT_VEC4(1, 1, 1, 1);
		VEKT_VEC4 focus_color	 = VEKT_VEC4(1, 1, 1, 1);
		VEKT_VEC4 disabled_color = VEKT_VEC4(1, 1, 1, 1);
	};

	struct rounding_props
	{
		f32			 rounding = 0.0f;
		unsigned int segments = 0;
	};
	struct font;

	struct text_props
	{
#ifdef VEKT_STRING_CSTR
		VEKT_STRING text = nullptr;
#else
		VEKT_STRING text = "";
#endif
		font*	 font = nullptr;
		uint64_t hash = 0;
#ifdef VEKT_STRING_CSTR
		size_t		 text_capacity = 0;
		unsigned int append_ctr	   = 0;
#endif
		f32			  scale	  = 1.0f;
		unsigned char spacing = 0;
	};

	struct widget_gfx
	{
		VEKT_VEC4	   color	  = VEKT_VEC4(1, 1, 1, 1);
		void*		   user_data  = nullptr;
		unsigned int   draw_order = 0;
		unsigned short flags	  = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	// :: WIDGET
	////////////////////////////////////////////////////////////////////////////////
	class builder;

	////////////////////////////////////////////////////////////////////////////////
	// :: VERTICES
	////////////////////////////////////////////////////////////////////////////////

	struct vertex
	{
		VEKT_VEC2 pos;
		VEKT_VEC2 uv;
		VEKT_VEC4 color;
	};

	////////////////////////////////////////////////////////////////////////////////
	// :: BUILDER
	////////////////////////////////////////////////////////////////////////////////

	enum class font_type
	{
		normal,
		sdf,
		lcd,
	};

	struct draw_buffer
	{
		vertex*		 vertex_start  = nullptr;
		index*		 index_start   = nullptr;
		void*		 user_data	   = nullptr;
		VEKT_VEC4	 clip		   = VEKT_VEC4();
		unsigned int atlas_id	   = NULL_WIDGET_ID;
		unsigned int font_id	   = NULL_WIDGET_ID;
		unsigned int draw_order	   = 0;
		unsigned int vertex_count  = 0;
		unsigned int index_count   = 0;
		unsigned int _max_vertices = 0;
		unsigned int _max_indices  = 0;
		font_type	 font_type	   = font_type::normal;

		inline void add_vertex(const vertex& vtx)
		{
			ASSERT(vertex_count < _max_vertices);
			vertex_start[vertex_count] = vtx;
			vertex_count++;
		}

		inline vertex& add_get_vertex()
		{
			ASSERT(vertex_count < _max_vertices);
			const unsigned int idx = vertex_count;
			vertex_count++;
			return vertex_start[idx];
		}

		inline vertex* add_get_vertex(unsigned int count)
		{
			ASSERT(vertex_count + count < _max_vertices);
			const unsigned int idx = vertex_count;
			vertex_count += count;
			return vertex_start + idx;
		}

		inline void add_index(index idx)
		{
			ASSERT(index_count < _max_indices);
			index_start[index_count] = idx;
			index_count++;
		}

		inline index* add_get_index(unsigned int count)
		{
			ASSERT(index_count + count < _max_indices);
			const unsigned int idx = index_count;
			index_count += count;
			return index_start + idx;
		}
	};

	struct snapshot
	{
		vector<vekt::draw_buffer> draw_buffers;
		vertex*					  vertices		= nullptr;
		index*					  indices		= nullptr;
		unsigned int			  _max_vertices = 0;
		unsigned int			  _max_indices	= 0;

		void init(size_t vertex_size, size_t index_size);
		void uninit();
		void copy(const vector<vekt::draw_buffer>& draw_buffers);
	};

	typedef void		(*draw_callback)(const draw_buffer& db, void* user_data);
	typedef const char* (*allocate_text_callback)(void* ud, size_t sz);
	typedef void		(*deallocate_text_callback)(void* ud, const char* ptr);

	class theme
	{
	public:
		static VEKT_VEC4 color_item_bg;
		static VEKT_VEC4 color_item_hover;
		static VEKT_VEC4 color_item_press;
		static VEKT_VEC4 color_panel_bg;
		static VEKT_VEC4 color_divider;
		static VEKT_VEC4 color_item_outline;
		static VEKT_VEC4 color_item_fg;
		static f32		 item_height;
		static f32		 item_spacing;
		static f32		 indent_horizontal;
		static f32		 margin_horizontal;
		static f32		 margin_vertical;
		static f32		 border_thickness;
		static f32		 outline_thickness;
	};

	struct hover_callback
	{
		widget_func		on_hover_begin	= nullptr;
		widget_func		on_hover_move	= nullptr;
		widget_func		on_hover_end	= nullptr;
		widget_func		on_focus_lost	= nullptr;
		focus_gain_func on_focus_gained = nullptr;
		unsigned char	is_hovered		= 0;
		unsigned char	is_pressing		= 0;
		unsigned char	is_focused		= 0;
	};

	struct mouse_callback
	{
		mouse_func on_mouse		  = nullptr;
		drag_func  on_drag		  = nullptr;
		wheel_func on_mouse_wheel = nullptr;
	};

	struct key_callback
	{
		key_func on_key = nullptr;
	};

	struct widget_meta
	{
		id		   parent = NULL_WIDGET_ID;
		vector<id> children;
	};

	struct widget_user_data
	{
		void* ptr = nullptr;
	};

	struct size_props
	{
		margins		   child_margins = margins();
		VEKT_VEC2	   size			 = VEKT_VEC2();
		f32			   spacing		 = 0.0f;
		unsigned short flags		 = 0;
	};

	struct size_result
	{
		VEKT_VEC2 size = VEKT_VEC2();
	};

	struct pos_result
	{
		VEKT_VEC2 pos = VEKT_VEC2();
	};

	struct pos_props
	{
		VEKT_VEC2	   pos			 = VEKT_VEC2();
		f32			   scroll_offset = 0.0f;
		unsigned short flags		 = 0;
	};

	struct scroll_props
	{
		id	scroll_parent = NULL_WIDGET_ID;
		f32 scroll_ratio  = 0.0f;
		f32 _max_scroll	  = 0.0f;
	};

	struct custom_passes
	{
		widget_func custom_pos_pass	 = nullptr;
		widget_func custom_size_pass = nullptr;
		widget_func custom_draw_pass = nullptr;
	};

	struct text_cache
	{
		uint64_t	 hash	   = 0;
		unsigned int vtx_start = 0;
		unsigned int idx_start = 0;
		unsigned int vtx_count = 0;

		static inline uint64_t hash_combine_64(uint64_t a, uint64_t b)
		{
			return a ^ (b + 0x9e3779b97f4a7c15ull + (a << 12) + (a >> 4));
		}

		static inline uint64_t hash_text_props(const vekt::text_props& text, const VEKT_VEC4& color)
		{
			uint64_t h = std::hash<VEKT_STRING>{}(text.text);
			h		   = hash_combine_64(h, std::hash<void*>{}(text.font));
			h		   = hash_combine_64(h, std::hash<f32>{}(text.scale));
			h		   = hash_combine_64(h, std::hash<unsigned char>{}(text.spacing));
			h		   = hash_combine_64(h, std::hash<f32>{}(color.x));
			h		   = hash_combine_64(h, std::hash<f32>{}(color.y));
			h		   = hash_combine_64(h, std::hash<f32>{}(color.z));
			h		   = hash_combine_64(h, std::hash<f32>{}(color.w));
			return h;
		}
	};

	class builder
	{

	public:
		struct line_props
		{
			VEKT_VEC2	 p0			= VEKT_VEC2();
			VEKT_VEC2	 p1			= VEKT_VEC2();
			VEKT_VEC4	 color		= VEKT_VEC4(1, 1, 1, 1);
			f32			 thickness	= 1.0f;
			unsigned int draw_order = 0;
			void*		 user_data	= nullptr;
		};

		struct line_aa_props
		{
			VEKT_VEC2	 p0			  = VEKT_VEC2();
			VEKT_VEC2	 p1			  = VEKT_VEC2();
			VEKT_VEC4	 color		  = VEKT_VEC4(1, 1, 1, 1);
			f32			 thickness	  = 1.0f;
			unsigned int aa_thickness = 1;
			unsigned int draw_order	  = 0;
			void*		 user_data	  = nullptr;
		};

		struct circle_props
		{
			VEKT_VEC2	 center		= VEKT_VEC2();
			f32			 radius		= 0.0f;
			VEKT_VEC4	 color		= VEKT_VEC4(1, 1, 1, 1);
			unsigned int segments	= 32;
			bool		 filled		= true;
			f32			 thickness	= 1.0f; // used when filled == false
			unsigned int draw_order = 0;
			void*		 user_data	= nullptr;
		};

		struct sphere_props
		{
			VEKT_VEC2	 center		= VEKT_VEC2();
			f32			 radius		= 0.0f;
			VEKT_VEC4	 color		= VEKT_VEC4(1, 1, 1, 1);
			unsigned int segments	= 32;
			unsigned int draw_order = 0;
			void*		 user_data	= nullptr;
		};

		struct rect_props
		{
			const widget_gfx& gfx;
			const VEKT_VEC2&  min;
			const VEKT_VEC2&  max;
			VEKT_VEC4		  color_start;
			VEKT_VEC4		  color_end;
			VEKT_VEC4		  stroke_col;
			direction		  color_direction;
			id				  widget_id = 0;
			bool			  multi_color;
		};

	public:
		struct init_config
		{
			unsigned int widget_count				 = 1024;
			size_t		 vertex_buffer_sz			 = 1024 * 1024;
			size_t		 index_buffer_sz			 = 1024 * 1024;
			size_t		 text_cache_vertex_buffer_sz = 1024 * 1024;
			size_t		 text_cache_index_buffer_sz	 = 1024 * 1024;
			size_t		 buffer_count				 = 10;
		};

		builder()					  = default;
		builder(const builder& other) = delete;
		~builder()
		{
		}

		void				init(const init_config& conf);
		void				uninit();
		void				build_begin(const VEKT_VEC2& screen_size);
		void				build_end();
		void				widget_set_size(id widget_id, const VEKT_VEC2& size, helper_size_type helper_x = helper_size_type::relative, helper_size_type helper_y = helper_size_type::relative);
		void				widget_set_size_abs(id widget_id, const VEKT_VEC2& size);
		void				widget_set_pos(id				  widget_id,
										   const VEKT_VEC2&	  pos,
										   helper_pos_type	  helper_x = helper_pos_type::relative,
										   helper_pos_type	  helper_y = helper_pos_type::relative,
										   helper_anchor_type anchor_x = helper_anchor_type::start,
										   helper_anchor_type anchor_y = helper_anchor_type::start);
		void				widget_set_pos_abs(id widget_id, const VEKT_VEC2& pos);
		const VEKT_VEC2&	widget_get_size(id widget_id) const;
		const VEKT_VEC2&	widget_get_pos(id widget_id) const;
		size_props&			widget_get_size_props(id widget_id);
		scroll_props&		widget_get_scroll_props(id widget_id);
		pos_props&			widget_get_pos_props(id widget_id);
		void				widget_set_scroll_offset(id widget_id, f32 offset);
		VEKT_VEC4			widget_get_clip(id widget_id) const;
		widget_gfx&			widget_get_gfx(id widget);
		stroke_props&		widget_get_stroke(id widget);
		rounding_props&		widget_get_rounding(id widget);
		aa_props&			widget_get_aa(id widget);
		second_color_props& widget_get_second_color(id widget);
		input_color_props&	widget_get_input_colors(id widget);
		text_props&			widget_get_text(id widget);
		unsigned int		widget_get_character_index(id widget, f32 x_diff);
		f32					widget_get_character_offset(id widget, unsigned int index);
		mouse_callback&		widget_get_mouse_callbacks(id widget);
		key_callback&		widget_get_key_callbacks(id widget);
		hover_callback&		widget_get_hover_callbacks(id widget);
		widget_user_data&	widget_get_user_data(id widget);
		custom_passes&		widget_get_custom_pass(id widget);
		const widget_meta&	widget_get_meta(id widget);
		id					widget_get_child(id widget, unsigned int index);
		void				widget_add_child(id widget_id, id child_id);
		void				widget_remove_child(id widget_id, id child_id);
		void				widget_set_text(id wg, const char* text, size_t default_text_capacity = 256);
		void				widget_append_text_start(id widget);
		void				widget_append_text(id widget, f32 f, int precision = 3, size_t default_text_capacity = 256);
		void				widget_append_text(id widget, unsigned int, size_t default_text_capacity = 256);
		void				widget_append_text(id widget, const char*, size_t default_text_capacity = 256);
		void				widget_update_text(id widget);
		void				widget_set_visible(id widget, bool is_visible);
		bool				widget_get_visible(id widget) const;
		void				on_mouse_move(const VEKT_VEC2& mouse, bool send_events = true);
		input_event_result	on_mouse_event(const mouse_event& ev);
		input_event_result	on_mouse_wheel_event(const mouse_wheel_event& ev);
		input_event_result	on_key_event(const key_event& ev);
		void				next_focus();
		void				prev_focus();
		void				add_line(const line_props& props);
		void				add_line_aa(const line_aa_props& props);
		void				add_circle(const circle_props& props);
		void				add_sphere(const sphere_props& props);
		void				add_filled_rect(const rect_props& props);
		void				add_filled_rect_aa(const rect_props& props);
		void				add_filled_rect_outline(const rect_props& props);
		void				add_filled_rect_rounding(const rect_props& props);
		void				add_filled_rect_aa_outline(const rect_props& props);
		void				add_filled_rect_aa_rounding(const rect_props& props);
		void				add_filled_rect_rounding_outline(const rect_props& props);
		void				add_filled_rect_aa_outline_rounding(const rect_props& props);
		void				add_stroke_rect(const rect_props& props);
		void				add_stroke_rect_aa(const rect_props& props);
		void				add_stroke_rect_rounding(const rect_props& props);
		void				add_stroke_rect_aa_rounding(const rect_props& props);
		void				add_text(const text_props& text, const VEKT_VEC4& color, const VEKT_VEC2& position, const VEKT_VEC2& size, unsigned int draw_order, void* user_data, bool flip_uv = false);
		void				add_text_cached(const text_props& text, const VEKT_VEC4& color, const VEKT_VEC2& position, const VEKT_VEC2& size, unsigned int draw_order, void* user_data);
		static VEKT_VEC2	get_text_size(const text_props& text, const VEKT_VEC2& parent_size = VEKT_VEC2());
		draw_buffer*		get_draw_buffer(unsigned int draw_order, void* user_data, font* fnt = nullptr);
		VEKT_VEC4			calculate_intersection(const VEKT_VEC4& clip0, const VEKT_VEC4& clip1) const;
		id					allocate();
		void				deallocate(id w);
		void				clear_text_cache();
		void				set_focus(id widget, bool from_nav);
		void				set_pressing(id widget, unsigned int button, const VEKT_VEC2& press_pos);
		void				build_hierarchy();

		// Widgets
		void widget_add_debug_wrap(id widget);

		inline VEKT_VEC4 get_current_clip() const
		{
			return _clip_stack.empty() ? VEKT_VEC4() : _clip_stack[_clip_stack.size() - 1].rect;
		}

		inline void set_on_draw(draw_callback cb, void* user_data)
		{
			_on_draw	= cb;
			_on_draw_ud = user_data;
		}

		inline id get_root() const
		{
			return _root;
		}

		inline const vector<draw_buffer>& get_draw_buffers() const
		{
			return _draw_buffers;
		}

		inline void set_callback_user_data(void* callback_user_data)
		{
			_callback_user_data = callback_user_data;
		}

		inline void set_on_allocate_text(allocate_text_callback cb)
		{
			_on_allocate_text = cb;
		}

		inline void set_on_deallocate_text(deallocate_text_callback cb)
		{
			_on_deallocate_text = cb;
		}

	private:
		unsigned int count_total_children(id widget_id) const;
		void		 populate_hierarchy(id current_widget_id, unsigned int depth);
		void		 calculate_sizes();
		void		 calculate_positions();
		void		 calculate_draw();
		void		 generate_rounded_rect(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& min, const VEKT_VEC2& max, f32 rounding, int segments);
		void		 generate_sharp_rect(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& min, const VEKT_VEC2& max);
		void		 generate_offset_rect_4points(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& min, const VEKT_VEC2& max, f32 amount);
		void		 generate_offset_rect(vector<VEKT_VEC2>& out_path, const vector<VEKT_VEC2>& base_path, f32 amount);
		void		 add_strip(draw_buffer* db, unsigned int outer_start, unsigned int inner_start, unsigned int size, bool add_ccw);
		void		 add_filled_rect(draw_buffer* db, unsigned int start);
		void		 add_filled_rect_central(draw_buffer* db, unsigned int start, unsigned int central_start, unsigned int size);
		void		 add_vertices(draw_buffer* db, const vector<VEKT_VEC2>& path, const VEKT_VEC4& color, const VEKT_VEC2& min, const VEKT_VEC2& max);
		void		 add_vertices_multicolor(draw_buffer* db, const vector<VEKT_VEC2>& path, const VEKT_VEC4& color_start, const VEKT_VEC4& color_end, direction direction, const VEKT_VEC2& min, const VEKT_VEC2& max);
		void		 add_central_vertex(draw_buffer* db, const VEKT_VEC4& color, const VEKT_VEC2& min, const VEKT_VEC2& max);
		void		 add_central_vertex_multicolor(draw_buffer* db, const VEKT_VEC4& color_start, const VEKT_VEC4& color_end, const VEKT_VEC2& min, const VEKT_VEC2& max);
		void		 add_vertices_aa(draw_buffer* db, const vector<VEKT_VEC2>& path, unsigned int original_vertices_idx, f32 alpha, const VEKT_VEC2& min, const VEKT_VEC2& max);
		void		 deallocate_impl(id widget);

		// Immediate helpers
		void generate_circle_path(vector<VEKT_VEC2>& out_path, const VEKT_VEC2& center, f32 radius, unsigned int segments);

	private:
		struct clip_info
		{
			VEKT_VEC4	 rect  = VEKT_VEC4();
			unsigned int depth = 0;
		};

		struct depth_first_child_info
		{
			id			 widget_id		= NULL_WIDGET_ID;
			unsigned int depth			= 0;
			unsigned int owned_children = 0;
		};

		struct arena
		{
			void*  base_ptr = nullptr;
			size_t capacity = 0;
		};

		vector<id>					   _free_list;
		vector<clip_info>			   _clip_stack;
		vector<draw_buffer>			   _draw_buffers;
		vector<id>					   _depth_first_widgets;
		vector<id>					   _depth_first_mouse_wheel_widgets;
		vector<id>					   _depth_first_fill_parents;
		vector<id>					   _depth_first_scrolls;
		vector<id>					   _depth_first_focusables;
		vector<id>					   _draw_order_first_mouse_widgets;
		vector<id>					   _draw_order_first_hover_widgets;
		vector<depth_first_child_info> _depth_first_child_info;
		vector<text_cache>			   _text_cache = {};
		vector<VEKT_VEC2>			   _reuse_outer_path;
		vector<VEKT_VEC2>			   _reuse_inner_path;
		vector<VEKT_VEC2>			   _reuse_outline_path;
		vector<VEKT_VEC2>			   _reuse_aa_outer_path;
		vector<VEKT_VEC2>			   _reuse_aa_inner_path;

		arena _layout_arena = {};
		arena _gfx_arena	= {};
		arena _misc_arena	= {};

		unsigned int _widget_head  = 0;
		unsigned int _widget_count = 0;

		allocate_text_callback	 _on_allocate_text		   = nullptr;
		deallocate_text_callback _on_deallocate_text	   = nullptr;
		draw_callback			 _on_draw				   = nullptr;
		void*					 _on_draw_ud			   = nullptr;
		void*					 _callback_user_data	   = nullptr;
		widget_meta*			 _metas					   = nullptr;
		widget_user_data*		 _user_datas			   = nullptr;
		size_props*				 _size_properties		   = {};
		input_color_props*		 _input_color_properties   = {};
		pos_props*				 _pos_properties		   = {};
		scroll_props*			 _scroll_properties		   = {};
		size_result*			 _size_results			   = {};
		pos_result*				 _pos_results			   = {};
		widget_gfx*				 _gfxs					   = {};
		stroke_props*			 _strokes				   = {};
		second_color_props*		 _second_colors			   = {};
		rounding_props*			 _roundings				   = {};
		aa_props*				 _aa_props				   = {};
		text_props*				 _texts					   = {};
		hover_callback*			 _hover_callbacks		   = {};
		mouse_callback*			 _mouse_callbacks		   = {};
		key_callback*			 _key_callbacks			   = {};
		custom_passes*			 _custom_passes			   = {};
		vertex*					 _vertex_buffer			   = nullptr;
		index*					 _index_buffer			   = nullptr;
		vertex*					 _text_cache_vertex_buffer = nullptr;
		index*					 _text_cache_index_buffer  = nullptr;
		VEKT_VEC2				 _mouse_position		   = {};
		size_t					 _total_sz				   = 0;
		unsigned int			 _vertex_count_per_buffer  = 0;
		unsigned int			 _index_count_per_buffer   = 0;
		unsigned int			 _buffer_count			   = 0;
		unsigned int			 _buffer_counter		   = 0;
		unsigned int			 _text_cache_vertex_count  = 0;
		unsigned int			 _text_cache_vertex_size   = 0;
		unsigned int			 _text_cache_index_count   = 0;
		unsigned int			 _text_cache_index_size	   = 0;
		id						 _root					   = NULL_WIDGET_ID;
		id						 _pressed_widget		   = NULL_WIDGET_ID;
		id						 _focused_widget		   = NULL_WIDGET_ID;
		VEKT_VEC2				 _press_relative_pos	   = VEKT_VEC2();
		unsigned int			 _pressed_button		   = 0;
	};

	////////////////////////////////////////////////////////////////////////////////
	// :: FONT & ATLAS & GLYPH
	////////////////////////////////////////////////////////////////////////////////

	class atlas;

	struct glyph
	{
		unsigned char* sdf_data			 = nullptr;
		int			   kern_advance[128] = {0};
		int			   width			 = 0;
		int			   height			 = 0;
		int			   advance_x		 = 0;
		int			   left_bearing		 = 0;
		f32			   x_offset			 = 0.0f;
		f32			   y_offset			 = 0.0f;
		int			   atlas_x			 = 0;
		int			   atlas_y			 = 0;
		f32			   uv_x				 = 0.0f;
		f32			   uv_y				 = 0.0f;
		f32			   uv_w				 = 0.0f;
		f32			   uv_h				 = 0.0f;
	};

	struct font
	{
		glyph		 glyph_info[128];
		atlas*		 _atlas					= nullptr;
		unsigned int _font_id				= NULL_WIDGET_ID;
		unsigned int _atlas_required_height = 0;
		unsigned int _atlas_pos				= 0;
		f32			 _scale					= 0.0f;
		int			 ascent					= 0;
		int			 descent				= 0;
		int			 line_gap				= 0;
		unsigned int size					= 0;
		font_type	 type					= font_type::normal;
		~font();
	};

	class atlas
	{
	public:
		struct slice
		{
			slice(unsigned int pos_y, unsigned int h) : pos(pos_y), height(h){};
			unsigned int pos	= 0;
			unsigned int height = 0;
		};

		atlas(unsigned int width, unsigned int height, bool is_lcd, unsigned int id);
		~atlas();

		bool add_font(font* font);
		void remove_font(font* font);
		bool empty()
		{
			return _fonts.empty();
		}
		inline unsigned int get_width() const
		{
			return _width;
		}
		inline unsigned int get_height() const
		{
			return _height;
		}
		inline unsigned char* get_data() const
		{
			return _data;
		}
		inline unsigned int get_data_size() const
		{
			return _data_size;
		}

		inline bool get_is_lcd() const
		{
			return _is_lcd;
		}

		inline void set_id(unsigned int i)
		{
			_id = i;
		}

		inline unsigned int get_id() const
		{
			return _id;
		}

	private:
		vector<slice*> _available_slices = {};
		vector<font*>  _fonts			 = {};
		unsigned int   _width			 = 0;
		unsigned int   _height			 = 0;
		unsigned char* _data			 = nullptr;
		unsigned int   _data_size		 = 0;
		unsigned int   _id				 = 0;
		bool		   _is_lcd			 = false;
	};

	class font_manager
	{
	public:
		typedef void (*atlas_cb)(atlas* atl, void* user_data);

		font_manager(){};
		~font_manager()
		{
			ASSERT(_atlases.empty());
			ASSERT(_fonts.empty());
		};

		void init();
		void uninit();

		font* load_font_from_file(const char* file, unsigned int size, unsigned int range_start = 32, unsigned int range_end = 128, font_type type = font_type::normal, int sdf_padding = 3, int sdf_edge = 128, f32 sdf_distance = 32.0f);
		font* load_font(unsigned char* data, unsigned int data_size, unsigned int size, unsigned int range0, unsigned int range1, font_type type = font_type::normal, int sdf_padding = 3, int sdf_edge = 128, f32 sdf_distance = 32.0f);
		void  unload_font(font* fnt);

		inline void set_callback_user_data(void* callback_user_data)
		{
			_callback_user_data = callback_user_data;
		}

		inline void set_atlas_created_callback(atlas_cb cb)
		{
			_atlas_created_cb = cb;
		}

		inline void set_atlas_updated_callback(atlas_cb cb)
		{
			_atlas_updated_cb = cb;
		}

		inline void set_atlas_destroyed_callback(atlas_cb cb)
		{
			_atlas_destroyed_cb = cb;
		}

	private:
		void find_atlas(font* fnt);

	private:
		vector<atlas*> _atlases;
		vector<font*>  _fonts;
		void*		   _callback_user_data = nullptr;
		atlas_cb	   _atlas_created_cb   = nullptr;
		atlas_cb	   _atlas_updated_cb   = nullptr;
		atlas_cb	   _atlas_destroyed_cb = nullptr;
	};

}
