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

#ifdef SFG_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#endif

namespace SFG
{

#ifdef SFG_PLATFORM_WINDOWS

#define SFG_MOUSE_0		 VK_LBUTTON
#define SFG_MOUSE_1		 VK_RBUTTON
#define SFG_MOUSE_2		 VK_MBUTTON
#define SFG_MOUSE_3		 VK_XBUTTON1
#define SFG_MOUSE_4		 VK_XBUTTON2
#define SFG_MOUSE_5		 VK_LBUTTON
#define SFG_MOUSE_6		 VK_LBUTTON
#define SFG_MOUSE_7		 VK_LBUTTON
#define SFG_MOUSE_LAST	 SFG_MOUSE_7
#define SFG_MOUSE_LEFT	 SFG_MOUSE_0
#define SFG_MOUSE_RIGHT	 SFG_MOUSE_1
#define SFG_MOUSE_MIDDLE SFG_MOUSE_2

#define SFG_GAMEPAD_A			  VK_PAD_A
#define SFG_GAMEPAD_B			  VK_PAD_B
#define SFG_GAMEPAD_X			  VK_PAD_X
#define SFG_GAMEPAD_Y			  VK_PAD_Y
#define SFG_GAMEPAD_LEFT_BUMPER	  VK_PAD_LSHOULDER
#define SFG_GAMEPAD_RIGHT_BUMPER  VK_PAD_RSHOULDER
#define SFG_GAMEPAD_BACK		  VK_PAD_BACK
#define SFG_GAMEPAD_START		  VK_PAD_START
#define SFG_GAMEPAD_GUIDE		  VK_PAD_START
#define SFG_GAMEPAD_LEFT_THUMB	  VK_PAD_LTHUMB_PRESS
#define SFG_GAMEPAD_RIGHT_THUMB	  VK_PAD_RTHUMB_PRESS
#define SFG_GAMEPAD_DPAD_UP		  VK_PAD_DPAD_UP
#define SFG_GAMEPAD_DPAD_RIGHT	  VK_PAD_DPAD_RIGHT
#define SFG_GAMEPAD_DPAD_DOWN	  VK_PAD_DPAD_DOWN
#define SFG_GAMEPAD_DPAD_LEFT	  VK_PAD_DPAD_LEFT
#define SFG_GAMEPAD_LEFT_TRIGGER  4
#define SFG_GAMEPAD_RIGHT_TRIGGER 5
#define SFG_GAMEPAD_LAST		  SFG_GAMEPAD_RIGHT_TRIGGER

#define SFG_KEY_UNKNOWN UINT16_MAX - 1
#define SFG_KEY_A		0x41
#define SFG_KEY_B		0x42
#define SFG_KEY_C		0x43
#define SFG_KEY_D		0x44
#define SFG_KEY_E		0x45
#define SFG_KEY_F		0x46
#define SFG_KEY_G		0x47
#define SFG_KEY_H		0x48
#define SFG_KEY_I		0x49
#define SFG_KEY_J		0x4A
#define SFG_KEY_K		0x4B
#define SFG_KEY_L		0x4C
#define SFG_KEY_M		0x4D
#define SFG_KEY_N		0x4E
#define SFG_KEY_O		0x4F
#define SFG_KEY_P		0x50
#define SFG_KEY_Q		0x51
#define SFG_KEY_R		0x52
#define SFG_KEY_S		0x53
#define SFG_KEY_T		0x54
#define SFG_KEY_U		0x55
#define SFG_KEY_V		0x56
#define SFG_KEY_W		0x57
#define SFG_KEY_X		0x58
#define SFG_KEY_Y		0x59
#define SFG_KEY_Z		0x5A

#define SFG_KEY_0 0x30
#define SFG_KEY_1 0x31
#define SFG_KEY_2 0x32
#define SFG_KEY_3 0x33
#define SFG_KEY_4 0x34
#define SFG_KEY_5 0x35
#define SFG_KEY_6 0x36
#define SFG_KEY_7 0x37
#define SFG_KEY_8 0x38
#define SFG_KEY_9 0x39

#define SFG_KEY_RETURN	  VK_RETURN
#define SFG_KEY_ESCAPE	  VK_ESCAPE
#define SFG_KEY_BACKSPACE VK_BACK
#define SFG_KEY_TAB		  VK_TAB
#define SFG_KEY_SPACE	  VK_SPACE

#define SFG_KEY_MINUS		  VK_OEM_MINUS
#define SFG_KEY_TILDE		  VK_OEM_3 // 41 // ?
#define SFG_KEY_ANGLE_BRACKET VK_OEM_102
#define SFG_KEY_COMMA		  VK_OEM_COMMA
#define SFG_KEY_PERIOD		  VK_OEM_PERIOD
#define SFG_KEY_SLASH		  VK_DIVIDE
#define SFG_KEY_CAPSLOCK	  VK_CAPITAL

#define SFG_KEY_F1	VK_F1
#define SFG_KEY_F2	VK_F2
#define SFG_KEY_F3	VK_F3
#define SFG_KEY_F4	VK_F4
#define SFG_KEY_F5	VK_F5
#define SFG_KEY_F6	VK_F6
#define SFG_KEY_F7	VK_F7
#define SFG_KEY_F8	VK_F8
#define SFG_KEY_F9	VK_F9
#define SFG_KEY_F10 VK_F10
#define SFG_KEY_F11 VK_F11
#define SFG_KEY_F12 VK_F12
#define SFG_KEY_F13 VK_F13
#define SFG_KEY_F14 VK_F14
#define SFG_KEY_F15 VK_F15

#define SFG_KEY_PRINTSCREEN	 VK_PRINT
#define SFG_KEY_SCROLLLOCK	 VK_SCROLL
#define SFG_KEY_PAUSE		 VK_PAUSE
#define SFG_KEY_INSERT		 VK_INSERT
#define SFG_KEY_HOME		 VK_HOME
#define SFG_KEY_PAGEUP		 VK_PRIOR
#define SFG_KEY_DELETE		 VK_DELETE
#define SFG_KEY_END			 VK_END
#define SFG_KEY_PAGEDOWN	 VK_NEXT
#define SFG_KEY_RIGHT		 VK_RIGHT
#define SFG_KEY_LEFT		 VK_LEFT
#define SFG_KEY_DOWN		 VK_DOWN
#define SFG_KEY_UP			 VK_UP
#define SFG_KEY_NUMLOCKCLEAR VK_CLEAR

#define SFG_KEY_KP_DECIMAL	VK_DECIMAL
#define SFG_KEY_KP_DIVIDE	VK_DIVIDE
#define SFG_KEY_KP_MULTIPLY VK_MULTIPLY
#define SFG_KEY_KP_MINUS	VK_OEM_MINUS
#define SFG_KEY_KP_PLUS		VK_OEM_PLUS
#define SFG_KEY_KP_ENTER	VK_RETURN
#define SFG_KEY_KP_1		VK_NUMPAD1
#define SFG_KEY_KP_2		VK_NUMPAD2
#define SFG_KEY_KP_3		VK_NUMPAD3
#define SFG_KEY_KP_4		VK_NUMPAD4
#define SFG_KEY_KP_5		VK_NUMPAD5
#define SFG_KEY_KP_6		VK_NUMPAD6
#define SFG_KEY_KP_7		VK_NUMPAD7
#define SFG_KEY_KP_8		VK_NUMPAD8
#define SFG_KEY_KP_9		VK_NUMPAD9
#define SFG_KEY_KP_0		VK_NUMPAD0

#define SFG_KEY_LCTRL  VK_LCONTROL
#define SFG_KEY_LSHIFT VK_LSHIFT
#define SFG_KEY_LALT   VK_LMENU
#define SFG_KEY_LGUI   VK_LMENU
#define SFG_KEY_RCTRL  VK_RCONTROL
#define SFG_KEY_RSHIFT VK_RSHIFT
#define SFG_KEY_RALT   VK_RMENU
#define SFG_KEY_RGUI   VK_RMENU

#endif

#ifdef SFG_PLATFORM_OSX

#define SFG_MOUSE_0		 0
#define SFG_MOUSE_1		 1
#define SFG_MOUSE_2		 2
#define SFG_MOUSE_3		 3
#define SFG_MOUSE_4		 4
#define SFG_MOUSE_5		 5
#define SFG_MOUSE_6		 6
#define SFG_MOUSE_7		 7
#define SFG_MOUSE_LAST	 SFG_MOUSE_7
#define SFG_MOUSE_LEFT	 SFG_MOUSE_0
#define SFG_MOUSE_RIGHT	 SFG_MOUSE_1
#define SFG_MOUSE_MIDDLE SFG_MOUSE_2

#define SFG_GAMEPAD_A			  0
#define SFG_GAMEPAD_B			  1
#define SFG_GAMEPAD_X			  2
#define SFG_GAMEPAD_Y			  3
#define SFG_GAMEPAD_LEFT_BUMPER	  4
#define SFG_GAMEPAD_RIGHT_BUMPER  5
#define SFG_GAMEPAD_BACK		  6
#define SFG_GAMEPAD_START		  7
#define SFG_GAMEPAD_GUIDE		  8
#define SFG_GAMEPAD_LEFT_THUMB	  9
#define SFG_GAMEPAD_RIGHT_THUMB	  10
#define SFG_GAMEPAD_DPAD_UP		  11
#define SFG_GAMEPAD_DPAD_RIGHT	  12
#define SFG_GAMEPAD_DPAD_DOWN	  13
#define SFG_GAMEPAD_DPAD_LEFT	  14
#define SFG_GAMEPAD_LEFT_TRIGGER  4
#define SFG_GAMEPAD_RIGHT_TRIGGER 5
#define SFG_GAMEPAD_LAST		  SFG_GAMEPAD_RIGHT_TRIGGER

#define SFG_KEY_UNKNOWN -1
#define SFG_KEY_A		0x00
#define SFG_KEY_B		0x0B
#define SFG_KEY_C		0x08
#define SFG_KEY_D		0x02
#define SFG_KEY_E		0x0E
#define SFG_KEY_F		0x03
#define SFG_KEY_G		0x05
#define SFG_KEY_H		0x04
#define SFG_KEY_I		0x22
#define SFG_KEY_J		0x26
#define SFG_KEY_K		0x28
#define SFG_KEY_L		0x25
#define SFG_KEY_M		0x2E
#define SFG_KEY_N		0x2D
#define SFG_KEY_O		0x1F
#define SFG_KEY_P		0x23
#define SFG_KEY_Q		0x0C
#define SFG_KEY_R		0x0F
#define SFG_KEY_S		0x01
#define SFG_KEY_T		0x11
#define SFG_KEY_U		0x20
#define SFG_KEY_V		0x09
#define SFG_KEY_W		0x0D
#define SFG_KEY_X		0x07
#define SFG_KEY_Y		0x10
#define SFG_KEY_Z		0x06

#define SFG_KEY_0 0x1D
#define SFG_KEY_1 0x12
#define SFG_KEY_2 0x13
#define SFG_KEY_3 0x14
#define SFG_KEY_4 0x15
#define SFG_KEY_5 0x17
#define SFG_KEY_6 0x16
#define SFG_KEY_7 0x1A
#define SFG_KEY_8 0x1C
#define SFG_KEY_9 0x19

#define SFG_KEY_RETURN	  0x24
#define SFG_KEY_ESCAPE	  0x35
#define SFG_KEY_BACKSPACE 0x33
#define SFG_KEY_TAB		  0x30
#define SFG_KEY_SPACE	  0x31

#define SFG_KEY_MINUS		  0x1B	// '-'
#define SFG_KEY_TILDE		  0x32	// '`'
#define SFG_KEY_ANGLE_BRACKET 0x102 // '`'
#define SFG_KEY_COMMA		  0x2B	// ','
#define SFG_KEY_PERIOD		  0x2F	// '.'
#define SFG_KEY_SLASH		  0x2C	// '/'
#define SFG_KEY_CAPSLOCK	  0x39

#define SFG_KEY_F1	0x7A
#define SFG_KEY_F2	0x78
#define SFG_KEY_F3	0x63
#define SFG_KEY_F4	0x76
#define SFG_KEY_F5	0x60
#define SFG_KEY_F6	0x61
#define SFG_KEY_F7	0x62
#define SFG_KEY_F8	0x64
#define SFG_KEY_F9	0x65
#define SFG_KEY_F10 0x6D
#define SFG_KEY_F11 0x67
#define SFG_KEY_F12 0x6F
#define SFG_KEY_F13 0x69
#define SFG_KEY_F14 0x6B
#define SFG_KEY_F15 0x71

#define SFG_KEY_PRINTSCREEN SFG_KEY_UNKNOWN
#define SFG_KEY_SCROLLLOCK	SFG_KEY_UNKNOWN
#define SFG_KEY_PAUSE		SFG_KEY_UNKNOWN

#define SFG_KEY_INSERT		 0x72 // "Help" key on some older Mac keyboards
#define SFG_KEY_HOME		 0x73
#define SFG_KEY_PAGEUP		 0x74
#define SFG_KEY_DELETE		 0x75
#define SFG_KEY_END			 0x77
#define SFG_KEY_PAGEDOWN	 0x79
#define SFG_KEY_RIGHT		 0x7C
#define SFG_KEY_LEFT		 0x7B
#define SFG_KEY_DOWN		 0x7D
#define SFG_KEY_UP			 0x7E
#define SFG_KEY_NUMLOCKCLEAR SFG_KEY_UNKNOWN

#define SFG_KEY_KP_DECIMAL	0x41
#define SFG_KEY_KP_DIVIDE	0x4B
#define SFG_KEY_KP_MULTIPLY 0x43
#define SFG_KEY_KP_MINUS	0x4E
#define SFG_KEY_KP_PLUS		0x45
#define SFG_KEY_KP_ENTER	0x4C
#define SFG_KEY_KP_1		0x53
#define SFG_KEY_KP_2		0x54
#define SFG_KEY_KP_3		0x55
#define SFG_KEY_KP_4		0x56
#define SFG_KEY_KP_5		0x57
#define SFG_KEY_KP_6		0x58
#define SFG_KEY_KP_7		0x59
#define SFG_KEY_KP_8		0x5B
#define SFG_KEY_KP_9		0x5C
#define SFG_KEY_KP_0		0x52

#define SFG_KEY_LCTRL  0x3B
#define SFG_KEY_LSHIFT 0x38
#define SFG_KEY_LALT   0x3A // Option key
#define SFG_KEY_LGUI   0x37 // Command key
#define SFG_KEY_RCTRL  0x3E
#define SFG_KEY_RSHIFT 0x3C
#define SFG_KEY_RALT   0x3D // Option key
#define SFG_KEY_RGUI   0x36 // Command key

#endif

	enum input_code : unsigned short
	{
		key_unknown		  = SFG_KEY_UNKNOWN,
		key_a			  = SFG_KEY_A,
		key_b			  = SFG_KEY_B,
		key_c			  = SFG_KEY_C,
		key_d			  = SFG_KEY_D,
		key_e			  = SFG_KEY_E,
		key_f			  = SFG_KEY_F,
		key_g			  = SFG_KEY_G,
		key_h			  = SFG_KEY_H,
		key_i			  = SFG_KEY_I,
		key_j			  = SFG_KEY_J,
		key_k			  = SFG_KEY_K,
		key_l			  = SFG_KEY_L,
		key_m			  = SFG_KEY_M,
		key_n			  = SFG_KEY_N,
		key_o			  = SFG_KEY_O,
		key_p			  = SFG_KEY_P,
		key_q			  = SFG_KEY_Q,
		key_r			  = SFG_KEY_R,
		key_s			  = SFG_KEY_S,
		key_t			  = SFG_KEY_T,
		key_u			  = SFG_KEY_U,
		key_v			  = SFG_KEY_V,
		key_w			  = SFG_KEY_W,
		key_x			  = SFG_KEY_X,
		key_y			  = SFG_KEY_Y,
		key_z			  = SFG_KEY_Z,
		key_alpha0		  = SFG_KEY_0,
		key_alpha1		  = SFG_KEY_1,
		key_alpha2		  = SFG_KEY_2,
		key_alpha3		  = SFG_KEY_3,
		key_alpha4		  = SFG_KEY_4,
		key_alpha5		  = SFG_KEY_5,
		key_alpha6		  = SFG_KEY_6,
		key_alpha7		  = SFG_KEY_7,
		key_alpha8		  = SFG_KEY_8,
		key_alpha9		  = SFG_KEY_9,
		key_return		  = SFG_KEY_RETURN,
		key_escape		  = SFG_KEY_ESCAPE,
		key_backspace	  = SFG_KEY_BACKSPACE,
		key_tab			  = SFG_KEY_TAB,
		key_space		  = SFG_KEY_SPACE,
		key_minus		  = SFG_KEY_MINUS,
		key_tilde		  = SFG_KEY_TILDE,
		key_angle_bracket = SFG_KEY_ANGLE_BRACKET,
		key_comma		  = SFG_KEY_COMMA,
		key_period		  = SFG_KEY_PERIOD,
		key_slash		  = SFG_KEY_SLASH,
		key_capslock	  = SFG_KEY_CAPSLOCK,
		key_f1			  = SFG_KEY_F1,
		key_f2			  = SFG_KEY_F2,
		key_f3			  = SFG_KEY_F3,
		key_f4			  = SFG_KEY_F4,
		key_f5			  = SFG_KEY_F5,
		key_f6			  = SFG_KEY_F6,
		key_f7			  = SFG_KEY_F7,
		key_f8			  = SFG_KEY_F8,
		key_f9			  = SFG_KEY_F9,
		key_f10			  = SFG_KEY_F10,
		key_f11			  = SFG_KEY_F11,
		key_f12			  = SFG_KEY_F12,
		key_printscreen	  = SFG_KEY_PRINTSCREEN,
		key_scrolllock	  = SFG_KEY_SCROLLLOCK,
		key_pause		  = SFG_KEY_PAUSE,
		key_insert		  = SFG_KEY_INSERT,
		key_home		  = SFG_KEY_HOME,
		key_pageup		  = SFG_KEY_PAGEUP,
		key_delete		  = SFG_KEY_DELETE,
		key_end			  = SFG_KEY_END,
		key_pagedown	  = SFG_KEY_PAGEDOWN,
		key_right		  = SFG_KEY_RIGHT,
		key_left		  = SFG_KEY_LEFT,
		key_down		  = SFG_KEY_DOWN,
		key_up			  = SFG_KEY_UP,
		key_numlock_clear = SFG_KEY_NUMLOCKCLEAR,
		keypad_decimal	  = SFG_KEY_KP_DECIMAL,
		keypad_divide	  = SFG_KEY_KP_DIVIDE,
		keypad_multiply	  = SFG_KEY_KP_MULTIPLY,
		keypad_minus	  = SFG_KEY_KP_MINUS,
		keypad_plus		  = SFG_KEY_KP_PLUS,
		keypad_enter	  = SFG_KEY_KP_ENTER,
		keypad_1		  = SFG_KEY_KP_1,
		keypad_2		  = SFG_KEY_KP_2,
		keypad_3		  = SFG_KEY_KP_3,
		keypad_4		  = SFG_KEY_KP_4,
		keypad_5		  = SFG_KEY_KP_5,
		keypad_6		  = SFG_KEY_KP_6,
		keypad_7		  = SFG_KEY_KP_7,
		keypad_8		  = SFG_KEY_KP_8,
		keypad_9		  = SFG_KEY_KP_9,
		keypad_0		  = SFG_KEY_KP_0,
		key_f13			  = SFG_KEY_F13,
		key_f14			  = SFG_KEY_F14,
		key_f15			  = SFG_KEY_F15,
		key_lctrl		  = SFG_KEY_LCTRL,
		key_lshift		  = SFG_KEY_LSHIFT,
		key_lalt		  = SFG_KEY_LALT,
		key_lgui		  = SFG_KEY_LGUI,
		key_rctrl		  = SFG_KEY_RCTRL,
		key_rshift		  = SFG_KEY_RSHIFT,
		key_ralt		  = SFG_KEY_RALT,
		key_rgui		  = SFG_KEY_RGUI,

		mouse_unknown = SFG_KEY_UNKNOWN,
		mouse_0		  = SFG_MOUSE_0,
		mouse_1		  = SFG_MOUSE_1,
		mouse_2		  = SFG_MOUSE_2,
		mouse_3		  = SFG_MOUSE_3,
		mouse_4		  = SFG_MOUSE_4,
		mouse_5		  = SFG_MOUSE_5,
		mouse_6		  = SFG_MOUSE_6,
		mouse_7		  = SFG_MOUSE_7,
		mouse_last	  = SFG_MOUSE_LAST,
		mouse_left	  = SFG_MOUSE_LEFT,
		mouse_right	  = SFG_MOUSE_RIGHT,
		mouse_middle  = SFG_MOUSE_MIDDLE,

		joystick_unknown = SFG_KEY_UNKNOWN,

		gamepad_unknown	   = SFG_KEY_UNKNOWN,
		gamepad_a		   = SFG_GAMEPAD_A,
		gamepad_b		   = SFG_GAMEPAD_B,
		gamepad_x		   = SFG_GAMEPAD_X,
		gamepad_y		   = SFG_GAMEPAD_Y,
		gamepad_l_bumper   = SFG_GAMEPAD_LEFT_BUMPER,
		gamepad_r_bumper   = SFG_GAMEPAD_RIGHT_BUMPER,
		gamepad_back	   = SFG_GAMEPAD_BACK,
		gamepad_start	   = SFG_GAMEPAD_START,
		gamepad_guide	   = SFG_GAMEPAD_GUIDE,
		gamepad_l_thumb	   = SFG_GAMEPAD_LEFT_THUMB,
		gamepad_r_thumb	   = SFG_GAMEPAD_RIGHT_THUMB,
		gamepad_dpad_up	   = SFG_GAMEPAD_DPAD_UP,
		gamepad_dpad_right = SFG_GAMEPAD_DPAD_RIGHT,
		gamepad_dpad_down  = SFG_GAMEPAD_DPAD_DOWN,
		gamepad_dpad_left  = SFG_GAMEPAD_DPAD_LEFT,
		gamepad_dpad_last  = SFG_GAMEPAD_LAST,
		gamepad_l_trigger  = SFG_GAMEPAD_LEFT_TRIGGER,
		gamepad_r_trigger  = SFG_GAMEPAD_RIGHT_TRIGGER,
		gamepad_last	   = SFG_GAMEPAD_LAST,
	};

} // namespace SFG
