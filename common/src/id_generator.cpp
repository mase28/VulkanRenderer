#include <common/id_generator.hpp>

uint32_t IdGenerator::get_unique_id() {
  if (!returned_ids_.empty()) {
    uint32_t id = returned_ids_.top();
    returned_ids_.pop();
    return id;
  }
  uint32_t id = cur_;
  cur_ += 1;
  return id;
}