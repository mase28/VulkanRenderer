#pragma once

#include <cstdint>
#include <functional>

template<typename Tag>
struct Handle {
  uint32_t value = UINT32_MAX;

  bool is_valid() const {return value != UINT32_MAX;}

  bool operator==(const Handle<Tag>& handle) const {return this->value == handle.value;}
};

namespace std {
  template<typename Tag>
  struct hash<Handle<Tag>> {
    size_t operator()(const Handle<Tag>& handle) const {
      return std::hash<uint32_t>{}(handle.value);
    }
  };
}