#ifdef _WIN64

#include "InputWin32.h"
#include "Input.h"

namespace lightning::input {
	namespace {
		constexpr u32 vk_mapping[256]{
			u32_invalid_id,					// 0x00
			InputCode::MOUSE_LEFT,			// 0x01
			InputCode::MOUSE_RIGHT,			// 0x02
			u32_invalid_id,					// 0x03
			InputCode::MOUSE_MIDDLE,		// 0x04
			u32_invalid_id,					// 0x05
			u32_invalid_id,					// 0x06
			u32_invalid_id,					// 0x07
			InputCode::KEY_BACKSPACE,		// 0x08
			InputCode::KEY_TAB,				// 0x09
			u32_invalid_id,					// 0x0A
			u32_invalid_id,					// 0x0B
			u32_invalid_id,					// 0x0C
			InputCode::KEY_RETURN,			// 0x0D
			u32_invalid_id,					// 0x0E
			u32_invalid_id,					// 0x0F

			InputCode::KEY_SHIFT,			// 0x10
			InputCode::KEY_CTRL,			// 0x11
			InputCode::KEY_ALT,				// 0x12
			InputCode::KEY_PAUSE,			// 0x13
			InputCode::KEY_CAPSLOCK,		// 0x14
			u32_invalid_id,					// 0x15
			u32_invalid_id,					// 0x16
			u32_invalid_id,					// 0x17
			u32_invalid_id,					// 0x18
			u32_invalid_id,					// 0x19
			u32_invalid_id,					// 0x1A
			InputCode::KEY_ESCAPE,			// 0x1B
			u32_invalid_id,					// 0x1C
			u32_invalid_id,					// 0x1D
			u32_invalid_id,					// 0x1E
			u32_invalid_id,					// 0x1F

			InputCode::KEY_SPACE,			// 0x20
			InputCode::KEY_PAGE_UP,			// 0x21
			InputCode::KEY_PAGE_DOWN,		// 0x22
			InputCode::KEY_END,				// 0x23
			InputCode::KEY_HOME,			// 0x24
			InputCode::KEY_LEFT,			// 0x25
			InputCode::KEY_UP,				// 0x26
			InputCode::KEY_RIGHT,			// 0x27
			InputCode::KEY_DOWN,			// 0x28
			u32_invalid_id,					// 0x29
			u32_invalid_id,					// 0x2A
			u32_invalid_id,					// 0x2B
			InputCode::KEY_PRINT_SCREEN,	// 0x2C
			InputCode::KEY_INSERT,			// 0x2D
			InputCode::KEY_DELETE,			// 0x2E
			u32_invalid_id,					// 0x2F

			InputCode::KEY_0,				// 0x30
			InputCode::KEY_1,				// 0x31
			InputCode::KEY_2,				// 0x32
			InputCode::KEY_3,				// 0x33
			InputCode::KEY_4,				// 0x34
			InputCode::KEY_5,				// 0x35
			InputCode::KEY_6,				// 0x36
			InputCode::KEY_7,				// 0x37
			InputCode::KEY_8,				// 0x38
			InputCode::KEY_9,				// 0x39
			u32_invalid_id,					// 0x3A
			u32_invalid_id,					// 0x3B
			u32_invalid_id,					// 0x3C
			u32_invalid_id,					// 0x3D
			u32_invalid_id,					// 0x3E
			u32_invalid_id,					// 0x3F

			u32_invalid_id,					// 0x40
			InputCode::KEY_A,				// 0x41
			InputCode::KEY_B,				// 0x42
			InputCode::KEY_C,				// 0x43
			InputCode::KEY_D,				// 0x44
			InputCode::KEY_E,				// 0x45
			InputCode::KEY_F,				// 0x46
			InputCode::KEY_G,				// 0x47
			InputCode::KEY_H,				// 0x48
			InputCode::KEY_I,				// 0x49
			InputCode::KEY_J,				// 0x4A
			InputCode::KEY_K,				// 0x4B
			InputCode::KEY_L,				// 0x4C
			InputCode::KEY_M,				// 0x4D
			InputCode::KEY_N,				// 0x4E
			InputCode::KEY_O,				// 0x4F

			InputCode::KEY_P,				// 0x50
			InputCode::KEY_Q,				// 0x51
			InputCode::KEY_R,				// 0x52
			InputCode::KEY_S,				// 0x53
			InputCode::KEY_T,				// 0x54
			InputCode::KEY_U,				// 0x55
			InputCode::KEY_V,				// 0x56
			InputCode::KEY_W,				// 0x57
			InputCode::KEY_X,				// 0x58
			InputCode::KEY_Y,				// 0x59
			InputCode::KEY_Z,				// 0x5A
			u32_invalid_id,					// 0x5B
			u32_invalid_id,					// 0x5C
			u32_invalid_id,					// 0x5D
			u32_invalid_id,					// 0x5E
			u32_invalid_id,					// 0x5F

			InputCode::KEY_NUMPAD_0,		// 0x60
			InputCode::KEY_NUMPAD_1,		// 0x61
			InputCode::KEY_NUMPAD_2,		// 0x62
			InputCode::KEY_NUMPAD_3,		// 0x63
			InputCode::KEY_NUMPAD_4,		// 0x64
			InputCode::KEY_NUMPAD_5,		// 0x65
			InputCode::KEY_NUMPAD_6,		// 0x66
			InputCode::KEY_NUMPAD_7,		// 0x67
			InputCode::KEY_NUMPAD_8,		// 0x68
			InputCode::KEY_NUMPAD_9,		// 0x69
			InputCode::KEY_MULTIPLY,		// 0x6A
			InputCode::KEY_ADD,				// 0x6B
			u32_invalid_id,					// 0x6C
			InputCode::KEY_SUBSTRACT,		// 0x6D
			InputCode::KEY_DECIMAL,			// 0x6E
			InputCode::KEY_DIVIDE,			// 0x6F

			InputCode::KEY_F1,				// 0x70
			InputCode::KEY_F2,				// 0x71
			InputCode::KEY_F3,				// 0x72
			InputCode::KEY_F4,				// 0x73
			InputCode::KEY_F5,				// 0x74
			InputCode::KEY_F6,				// 0x75
			InputCode::KEY_F7,				// 0x76
			InputCode::KEY_F8,				// 0x77
			InputCode::KEY_F9,				// 0x78
			InputCode::KEY_F10,				// 0x79
			InputCode::KEY_F11,				// 0x7A
			InputCode::KEY_F12,				// 0x7B
			u32_invalid_id,					// 0x7C
			u32_invalid_id,					// 0x7D
			u32_invalid_id,					// 0x7E
			u32_invalid_id,					// 0x7F

			u32_invalid_id,					// 0x80
			u32_invalid_id,					// 0x81
			u32_invalid_id,					// 0x82
			u32_invalid_id,					// 0x83
			u32_invalid_id,					// 0x84
			u32_invalid_id,					// 0x85
			u32_invalid_id,					// 0x86
			u32_invalid_id,					// 0x87
			u32_invalid_id,					// 0x88
			u32_invalid_id,					// 0x89
			u32_invalid_id,					// 0x8A
			u32_invalid_id,					// 0x8B
			u32_invalid_id,					// 0x8C
			u32_invalid_id,					// 0x8D
			u32_invalid_id,					// 0x8E
			u32_invalid_id,					// 0x8F

			InputCode::KEY_NUMLOCK,			// 0x90
			InputCode::KEY_SCROLLOCK,		// 0x91
			u32_invalid_id,					// 0x92
			u32_invalid_id,					// 0x93
			u32_invalid_id,					// 0x94
			u32_invalid_id,					// 0x95
			u32_invalid_id,					// 0x96
			u32_invalid_id,					// 0x97
			u32_invalid_id,					// 0x98
			u32_invalid_id,					// 0x99
			u32_invalid_id,					// 0x9A
			u32_invalid_id,					// 0x9B
			u32_invalid_id,					// 0x9C
			u32_invalid_id,					// 0x9D
			u32_invalid_id,					// 0x9E
			u32_invalid_id,					// 0x9F

			u32_invalid_id,					// 0xA0
			u32_invalid_id,					// 0xA1
			u32_invalid_id,					// 0xA2
			u32_invalid_id,					// 0xA3
			u32_invalid_id,					// 0xA4
			u32_invalid_id,					// 0xA5
			u32_invalid_id,					// 0xA6
			u32_invalid_id,					// 0xA7
			u32_invalid_id,					// 0xA8
			u32_invalid_id,					// 0xA9
			u32_invalid_id,					// 0xAA
			u32_invalid_id,					// 0xAB
			u32_invalid_id,					// 0xAC
			u32_invalid_id,					// 0xAD
			u32_invalid_id,					// 0xAE
			u32_invalid_id,					// 0xAF

			u32_invalid_id,					// 0xB0
			u32_invalid_id,					// 0xB1
			u32_invalid_id,					// 0xB2
			u32_invalid_id,					// 0xB3
			u32_invalid_id,					// 0xB4
			u32_invalid_id,					// 0xB5
			u32_invalid_id,					// 0xB6
			u32_invalid_id,					// 0xB7
			u32_invalid_id,					// 0xB8
			u32_invalid_id,					// 0xB9
			InputCode::KEY_COLON,			// 0xBA
			InputCode::KEY_PLUS,			// 0xBB
			InputCode::KEY_COMMA,			// 0xBC
			InputCode::KEY_MINUS,			// 0xBD
			InputCode::KEY_PERIOD,			// 0xBE
			InputCode::KEY_QUESTION,		// 0xBF

			InputCode::KEY_TILDE,			// 0xC0
			u32_invalid_id,					// 0xC1
			u32_invalid_id,					// 0xC2
			u32_invalid_id,					// 0xC3
			u32_invalid_id,					// 0xC4
			u32_invalid_id,					// 0xC5
			u32_invalid_id,					// 0xC6
			u32_invalid_id,					// 0xC7
			u32_invalid_id,					// 0xC8
			u32_invalid_id,					// 0xC9
			u32_invalid_id,					// 0xCA
			u32_invalid_id,					// 0xCB
			u32_invalid_id,					// 0xCC
			u32_invalid_id,					// 0xCD
			u32_invalid_id,					// 0xCE
			u32_invalid_id,					// 0xCF

			u32_invalid_id,					// 0xD0
			u32_invalid_id,					// 0xD1
			u32_invalid_id,					// 0xD2
			u32_invalid_id,					// 0xD3
			u32_invalid_id,					// 0xD4
			u32_invalid_id,					// 0xD5
			u32_invalid_id,					// 0xD6
			u32_invalid_id,					// 0xD7
			u32_invalid_id,					// 0xD8
			u32_invalid_id,					// 0xD9
			u32_invalid_id,					// 0xDA
			InputCode::KEY_BRACKET_OPEN,	// 0xDB
			InputCode::KEY_PIPE,			// 0xDC
			InputCode::KEY_BRACKET_CLOSE,	// 0xDD
			InputCode::KEY_QUOTE,			// 0xDE
			u32_invalid_id,					// 0xDF

			u32_invalid_id,					// 0xE0
			u32_invalid_id,					// 0xE1
			u32_invalid_id,					// 0xE2
			u32_invalid_id,					// 0xE3
			u32_invalid_id,					// 0xE4
			u32_invalid_id,					// 0xE5
			u32_invalid_id,					// 0xE6
			u32_invalid_id,					// 0xE7
			u32_invalid_id,					// 0xE8
			u32_invalid_id,					// 0xE9
			u32_invalid_id,					// 0xEA
			u32_invalid_id,					// 0xEB
			u32_invalid_id,					// 0xEC
			u32_invalid_id,					// 0xED
			u32_invalid_id,					// 0xEE
			u32_invalid_id,					// 0xEF

			u32_invalid_id,					// 0xF0
			u32_invalid_id,					// 0xF1
			u32_invalid_id,					// 0xF2
			u32_invalid_id,					// 0xF3
			u32_invalid_id,					// 0xF4
			u32_invalid_id,					// 0xF5
			u32_invalid_id,					// 0xF6
			u32_invalid_id,					// 0xF7
			u32_invalid_id,					// 0xF8
			u32_invalid_id,					// 0xF9
			u32_invalid_id,					// 0xFA
			u32_invalid_id,					// 0xFB
			u32_invalid_id,					// 0xFC
			u32_invalid_id,					// 0xFD
			u32_invalid_id,					// 0xFE
			u32_invalid_id,					// 0xFF
		};

		struct ModifierFlags {
			enum Flags : u8 {
				LEFT_SHIFT = 0x10,
				LEFT_CONTROL = 0X20,
				LEFT_ALT = 0X40,

				RIGHT_SHIFT = 0x01,
				RIGHT_CONTROL = 0X02,
				RIGHT_ALT = 0X04,
			};
		};

		u8 modifier_key_state{ 0 };

		void set_modifier_input(u8 virtual_key, InputCode::Code code, ModifierFlags::Flags flags) {
			if (GetKeyState(virtual_key) < 0) {
				set(InputSource::KEYBOARD, code, { 1.f, 0.f, 0.f });
				modifier_key_state |= flags;
			}
			else if (modifier_key_state & flags) {
				set(InputSource::KEYBOARD, code, { 0.f, 0.f, 0.f });
				modifier_key_state &= ~flags;
			}
		}

		void set_modifier_inputs(InputCode::Code code) {
			if (code == InputCode::KEY_SHIFT) {
				set_modifier_input(VK_LSHIFT, InputCode::KEY_LEFT_SHIFT, ModifierFlags::LEFT_SHIFT);
				set_modifier_input(VK_RSHIFT, InputCode::KEY_RIGHT_SHIFT, ModifierFlags::RIGHT_SHIFT);
			}
			else if (code == InputCode::KEY_CTRL) {
				set_modifier_input(VK_LCONTROL, InputCode::KEY_LEFT_CTRL, ModifierFlags::LEFT_CONTROL);
				set_modifier_input(VK_RCONTROL, InputCode::KEY_RIGHT_CTRL, ModifierFlags::RIGHT_CONTROL);
			}
			else if(code == InputCode::KEY_ALT) {
				set_modifier_input(VK_LMENU, InputCode::KEY_LEFT_ALT, ModifierFlags::LEFT_ALT);
				set_modifier_input(VK_RMENU, InputCode::KEY_RIGHT_ALT, ModifierFlags::RIGHT_ALT);
			}
		}

		constexpr math::v2 get_mouse_position(LPARAM lparam) {
			return { (f32)((s16)(lparam & 0x0000FFFF)), (f32)((s16)(lparam >> 16)) };
		}
	}

	HRESULT process_input_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		switch (msg) {
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN: {
				assert(wparam <= 0xFF);
				const InputCode::Code code{ vk_mapping[wparam & 0xFF] };
				if (code != u32_invalid_id) {
					set(InputSource::KEYBOARD, code, { 1.f, 0.f, 0.f });
				}
			}
			break;

			case WM_KEYUP:
			case WM_SYSKEYUP: {
				assert(wparam <= 0xFF);
				const InputCode::Code code{ vk_mapping[wparam & 0xFF] };
				if (code != u32_invalid_id) {
					set(InputSource::KEYBOARD, code, { 0.f, 0.f, 0.f });
				}

			}
			break;

			case WM_MOUSEMOVE: {
				const math::v2 pos{ get_mouse_position(lparam) };
				set(InputSource::MOUSE, InputCode::MOUSE_POSITION_X, { pos.x, 0.f, 0.f });
				set(InputSource::MOUSE, InputCode::MOUSE_POSITION_Y, { pos.y, 0.f, 0.f });
				set(InputSource::MOUSE, InputCode::MOUSE_POSITION, { pos.x, pos.y, 0.f });
			}
			break;

			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN: {
				SetCapture(hwnd);
				const InputCode::Code code{ msg == WM_LBUTTONDOWN ? InputCode::MOUSE_LEFT : msg == WM_RBUTTONDOWN ? InputCode::MOUSE_RIGHT : InputCode::MOUSE_MIDDLE };
				const math::v2 pos{ get_mouse_position(lparam) };
				set(InputSource::MOUSE, code, { pos.x, pos.y, 1.f });
			}
			break;

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP: {
				ReleaseCapture();
				const InputCode::Code code{ msg == WM_LBUTTONUP ? InputCode::MOUSE_LEFT : msg == WM_RBUTTONUP ? InputCode::MOUSE_RIGHT : InputCode::MOUSE_MIDDLE };
				const math::v2 pos{ get_mouse_position(lparam) };
				set(InputSource::MOUSE, code, { pos.x, pos.y, 0.f });
			}
			break;

			case WM_MOUSEWHEEL: {
				set(InputSource::MOUSE, InputCode::MOUSE_WHEEL, { (f32)(GET_WHEEL_DELTA_WPARAM(wparam)), 0.f, 0.f });
			}
			break;
		}

		return S_OK;
	}
}

#endif