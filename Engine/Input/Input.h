#pragma once

#include "CommonHeaders.h"
#include "EngineAPI/Input.h"

namespace lightning::input {
	void bind(InputSource source);
	void unbind(InputSource::Type type, InputCode::Code code);
	void unbind(u64 binding);
	void set(InputSource::Type type, InputCode::Code code, math::v3 value);
}