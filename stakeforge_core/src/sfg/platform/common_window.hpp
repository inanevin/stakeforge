/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/data/bitmask.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>

namespace sfg
{
	struct window_event_t;
	struct window_runtime_t;
	typedef void (*window_event_callback)(void* hwnd, const window_event_t& ev, void* user_data);
	typedef bool (*window_client_hit_test_callback)(window_runtime_t& runtime, const vec2i16_t& pos, void* user_data);

	struct monitor_info_t
	{
		vec2i16_t position	  = vec2i16_t::zero;
		vec2u16_t size		  = vec2u16_t::zero;
		vec2u16_t work_size	  = vec2u16_t::zero;
		u64		  device_hash = 0;
		u32		  dpi		  = 0;
		f32		  dpi_scale	  = 0.0f;
		bool	  is_primary  = false;
	};

	enum class window_style_e : u8
	{
		app_window,
		borderless,
		alpha,
	};

	enum class window_cursor_confinement_e : u8
	{
		none,
		window,
		pointer,
		position,
		center,
	};

	enum window_event_flags
	{
		wef_high_freq = 1 << 0,
	};

	enum class window_cursor_state_e : u8
	{
		arrow,
		hand,
		resize_hr,
		resize_vt,
		resize_nwse,
		resize_nesw,
		caret,
	};
	enum class window_event_type_e : u8
	{
		key = 0,
		mouse,
		wheel,
		delta,
		focus,
		display_change,
		resize,
		repos,
	};

	enum class window_event_sub_type_e : u8
	{
		press,
		release,
		repeat,
	};

	struct window_event_t
	{
		vec2i16_t				value = vec2i16_t::zero;
		u16						button;
		window_event_type_e		type	 = window_event_type_e::key;
		window_event_sub_type_e sub_type = window_event_sub_type_e::press;
		bitmask_t<u8>			flags	 = 0;
	};

	enum class window_runtime_flags_e : u8
	{
		has_focus			 = 1 << 0,
		close_requested		 = 1 << 1,
		high_frequency_input = 1 << 2,
		minimized			 = 1 << 3,
		maximized			 = 1 << 4,
	};

	struct window_runtime_t
	{
		monitor_info_t					monitor_info				= {};
		window_event_callback			event_callback				= nullptr;
		void*							event_callback_user_data	= nullptr;
		window_client_hit_test_callback client_hit_test_callback	= nullptr;
		void*							client_hit_test_user_data	= nullptr;
		void*							window_handle				= nullptr;
		void*							platform_handle				= nullptr;
		vec2i16_t						pos							= vec2i16_t::zero;
		vec2u16_t						size						= vec2u16_t::zero;
		vec2u16_t						true_size					= vec2u16_t::zero;
		vec2i16_t						mouse_position_abs			= vec2i16_t::zero;
		vec2i16_t						mouse_position				= vec2i16_t::zero;
		vec2i16_t						cursor_confinement_position = vec2i16_t::zero;
		gfx_handle_t					swapchain					= {};
		window_style_e					style						= window_style_e::app_window;
		window_cursor_confinement_e		cursor_confinement			= window_cursor_confinement_e::none;
		bitmask_t<u8>					flags						= 0;
		bool							is_hidden					= false;
		bool							cursor_visible				= true;

		inline bool has_flag(window_runtime_flags_e flag) const
		{
			return flags.is_set(static_cast<u8>(flag));
		}

		inline void set_flag(window_runtime_flags_e flag, bool is_set = true)
		{
			flags.set(static_cast<u8>(flag), is_set);
		}

		inline void remove_flag(window_runtime_flags_e flag)
		{
			flags.remove(static_cast<u8>(flag));
		}
	};

}
