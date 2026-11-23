#pragma once

#include <cstdint>
#include <stack>

class IdGenerator {
public:
  uint32_t get_unique_id();
  void return_id(uint32_t id) {returned_ids_.push(id);}
private:
  uint32_t cur_ = 0;
  std::stack<uint32_t> returned_ids_;
};