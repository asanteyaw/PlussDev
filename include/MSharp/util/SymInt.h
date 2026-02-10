#pragma once

#include <cstdint>

namespace MSharp {

class SymInt {
 public:
  SymInt() : value_(0) {}
  explicit SymInt(int64_t v) : value_(v) {}
  int64_t toInt() const { return value_; }
  bool is_heap_allocated() const { return false; }
  int64_t guard_int(const char* /*file*/, int64_t /*line*/) const { return value_; }
  static bool check_range(long /*v*/) { return true; }
 private:
  int64_t value_ = 0;
};

} // namespace MSharp
