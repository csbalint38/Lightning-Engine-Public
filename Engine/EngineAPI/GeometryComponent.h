#pragma once
#include "../Components/ComponentsCommonHeaders.h"

namespace lightning::geometry {
  DEFINE_TYPED_ID(geometry_id);

  class Component final {
    public:
      constexpr explicit Component(geometry_id id) : _id{ id } {}
      constexpr Component() : _id{ id::invalid_id } {}
      constexpr geometry_id get_id() const { return _id; }
      constexpr bool is_valid() const { return id::is_valid(_id); }

    private:
      geometry_id _id;
    };
}