#pragma once

#include "../Components/ComponentsCommonHeaders.h"

namespace lightning::script {
	DEFINE_TYPED_ID(script_id);

	class Component final {
	private:
		script_id _id;
	public:
		constexpr explicit Component(script_id id) : _id{ id } {}
		constexpr Component() : _id{ id::invalid_id } {}
		constexpr script_id get_id() const { return _id; }
		constexpr bool is_valid() const { return id::is_valid(_id); }
	};
}