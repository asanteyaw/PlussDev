#pragma once
#include <cstdint>
#include <string>
#include <string_view>
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
  // Python-style traceback from raw return addresses captured at the throw site. Frames run oldest to
  // newest, each as File "path", line N, in function followed by the source line; the newest frame is
  // always the given raise location, so it is exact even when the binary has no debug information.
  static std::string format_traceback(
      const void* const* addrs,
      std::size_t count,
      const char* raise_file,
      std::uint32_t raise_line,
      const char* raise_function,
      const char* raise_source = nullptr);
  static std::size_t capture_addresses(void** out, std::size_t capacity);
  // The pieces of the traceback that depend only on text, so they can be checked on any platform. `shorten_path`
  // shows a source file relative to the repository: `this_file` is the path this source file was compiled as
  // (its own __FILE__), whatever the separators or the drive letter of the machine that built it. `parse_addr2line`
  // reads one frame of GNU addr2line output (a function line and a "file:line" line, which may end in
  // " (discriminator N)"); it returns false for an unresolved frame ("??").
  static std::string shorten_path(std::string_view file, std::string_view this_file);
  static bool parse_addr2line(std::string_view function_line, std::string_view location_line, Frame& out);
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
