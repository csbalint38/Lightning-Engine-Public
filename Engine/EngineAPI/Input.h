#pragma once

#include "CommonHeaders.h"

namespace lightning::input {
	struct Axis {
		enum Type : u32 {
			X = 0,
			Y = 1,
			Z = 2,
		};
	};

	struct ModifierKey {
		enum Key : u32 {
			NONE = 0x00,
			LEFT_SHIFT = 0x01,
			RIGHT_SHIFT = 0x02,
			SHIFT = LEFT_SHIFT | RIGHT_SHIFT,
			LEFT_CTRL = 0x04,
			RIGHT_CTRL = 0x08,
			CTRL = LEFT_CTRL | RIGHT_CTRL,
			LEFT_ALT = 0x10,
			RIGHT_ALT = 0x20,
			ALT = LEFT_ALT | RIGHT_ALT,
		};
	};

	struct InputValue {
		math::v3 previous;
		math::v3 current;
	};

	struct InputCode {
		enum Code : u32 {
			MOUSE_POSITION,
			MOUSE_POSITION_X,
			MOUSE_POSITION_Y,
			MOUSE_LEFT,
			MOUSE_RIGHT,
			MOUSE_MIDDLE,
			MOUSE_WHEEL,

			KEY_BACKSPACE,
			KEY_TAB,
			KEY_RETURN,
			KEY_SHIFT,
			KEY_LEFT_SHIFT,
			KEY_RIGHT_SHIFT,
			KEY_CTRL,
			KEY_LEFT_CTRL,
			KEY_RIGHT_CTRL,
			KEY_ALT,
			KEY_LEFT_ALT,
			KEY_RIGHT_ALT,
			KEY_PAUSE,
			KEY_CAPSLOCK,
			KEY_ESCAPE,
			KEY_SPACE,
			KEY_PAGE_UP,
			KEY_PAGE_DOWN,
			KEY_HOME,
			KEY_END,
			KEY_LEFT,
			KEY_UP,
			KEY_RIGHT,
			KEY_DOWN,
			KEY_PRINT_SCREEN,
			KEY_INSERT,
			KEY_DELETE,

			KEY_0,
			KEY_1,
			KEY_2,
			KEY_3,
			KEY_4,
			KEY_5,
			KEY_6,
			KEY_7,
			KEY_8,
			KEY_9,

			KEY_A,
			KEY_B,
			KEY_C,
			KEY_D,
			KEY_E,
			KEY_F,
			KEY_G,
			KEY_H,
			KEY_I,
			KEY_J,
			KEY_K,
			KEY_L,
			KEY_M,
			KEY_N,
			KEY_O,
			KEY_P,
			KEY_Q,
			KEY_R,
			KEY_S,
			KEY_T,
			KEY_U,
			KEY_V,
			KEY_W,
			KEY_X,
			KEY_Y,
			KEY_Z,

			KEY_NUMPAD_0,
			KEY_NUMPAD_1,
			KEY_NUMPAD_2,
			KEY_NUMPAD_3,
			KEY_NUMPAD_4,
			KEY_NUMPAD_5,
			KEY_NUMPAD_6,
			KEY_NUMPAD_7,
			KEY_NUMPAD_8,
			KEY_NUMPAD_9,

			KEY_MULTIPLY,
			KEY_ADD,
			KEY_SUBSTRACT,
			KEY_DECIMAL,
			KEY_DIVIDE,

			KEY_F1,
			KEY_F2,
			KEY_F3,
			KEY_F4,
			KEY_F5,
			KEY_F6,
			KEY_F7,
			KEY_F8,
			KEY_F9,
			KEY_F10,
			KEY_F11,
			KEY_F12,

			KEY_NUMLOCK,
			KEY_SCROLLOCK,
			KEY_COLON,
			KEY_PLUS,
			KEY_COMMA,
			KEY_MINUS,
			KEY_PERIOD,
			KEY_QUESTION,
			KEY_BRACKET_OPEN,
			KEY_PIPE,
			KEY_BRACKET_CLOSE,
			KEY_QUOTE,
			KEY_TILDE,
		};
	};

	struct InputSource {
		enum Type : u32 {
			KEYBOARD,
			MOUSE,
			CONTROLLER,
			RAW,

			count
		};

		u64 binding{ 0 };
		Type source_type{};
		u32 code{ 0 };
		f32 multiplier{ 0 };
		bool is_discrete{ true };
		Axis::Type source_axis{};
		Axis::Type axis{};
		ModifierKey::Key modifier{};
	};

	void get(InputSource::Type type, InputCode::Code code, InputValue& value);
	void get(u64 binding, InputValue& value);

	namespace detail {
		class InputSystemBase {
			public:
				virtual void on_event(InputSource::Type, InputCode::Code, const InputValue&) = 0;
				virtual void on_event(u64, const InputValue&) = 0;

			protected:
				InputSystemBase();
				~InputSystemBase();
		};
	}

	template<typename T> class InputSystem final : public detail::InputSystemBase {
		public:
			using input_callback_t = void(T::*)(InputSource::Type, InputCode::Code, const InputValue&);
			using binding_callback_t = void(T::*)(u64, const InputValue&);

			void add_handler(InputSource::Type type, T* instance, input_callback_t callback) {
				assert(instance && callback && type < InputSource::count);
				auto& collection = _input_callbacks[type];
				for (const auto& func : collection) {
					if (func.instance == instance && func.callback == callback) return;
				}

				collection.emplace_back(InputCallback{ instance, callback });
			}

			void add_handler(u64 binding, T* instance, binding_callback_t callback) {
				assert(instance && callback);

				for (const auto& func : _binding_callbacks) {
					if (func.binding == binding && func.instance == instance && func.callback == callback) return;
				}

				_binding_callbacks.emplace_back(BindingCallback{ binding, instance, callback });
			}

			void on_event(InputSource::Type type, InputCode::Code code, const InputValue& value) override {
				assert(type < InputSource::count);
				for (const auto& item : _input_callbacks[type]) {
					(item.instance->*item.callback)(type, code, value);
				}
			}

			void on_event(u64 binding, const InputValue& value) override {
				for (const auto& item : _binding_callbacks) {
					if (item.binding == binding) {
						(item.instance->*item.callback)(binding, value);
					}
				}
			}

		private:
			struct InputCallback {
				T* instance;
				input_callback_t callback;
			};

			struct BindingCallback {
				u64 binding;
				T* instance;
				binding_callback_t callback;
			};

			util::vector<InputCallback> _input_callbacks[InputSource::count];
			util::vector<BindingCallback> _binding_callbacks;
	};
}