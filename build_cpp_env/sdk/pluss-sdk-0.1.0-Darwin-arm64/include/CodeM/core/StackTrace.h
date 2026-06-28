#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <source_location>

namespace pluss::codem {

struct Frame {
  std::string function;
  std::string file;
  std::uint32_t line{0};
};

class StackTrace {
public:
  static void push(const std::source_location& loc);
  static void pop();
  static std::string format(std::size_t max_frames = 64);
  static std::string format_native(std::size_t max_frames = 64);
  static std::string format_user_to_trigger(
      std::size_t max_frames = 128,
      std::size_t max_user_frames = 12);

private:
  static thread_local std::vector<Frame> frames_;
};

class CallGuard {
public:
  explicit CallGuard(const std::source_location& loc = std::source_location::current()) { StackTrace::push(loc); }
  ~CallGuard() { StackTrace::pop(); }
  CallGuard(const CallGuard&) = delete;
  CallGuard& operator=(const CallGuard&) = delete;
};

} // namespace pluss::codem
