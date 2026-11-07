#include "pluss/bi/detail/browser_launch.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(__APPLE__) || defined(__linux__)
#include <spawn.h>
#include <unistd.h>
extern char** environ;
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

namespace pluss::bi::detail {

namespace {

std::string sanitize_filename(std::string_view name) {
  std::string out;
  out.reserve(name.size());
  for (const char c : name) {
    const bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_';
    out.push_back(safe ? c : '_');
  }
  if (out.empty()) out = "dashboard";
  return out;
}

} // namespace

std::string open_html_in_browser(const std::string& html, std::string_view suggested_filename) {
  std::error_code ec;
  const auto temp_dir = std::filesystem::temp_directory_path(ec);
  if (ec) return {};

  const auto pid = static_cast<unsigned long>(
#if defined(_WIN32)
      ::GetCurrentProcessId()
#else
      ::getpid()
#endif
  );
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();

  std::ostringstream name;
  name << "plussbi_" << sanitize_filename(suggested_filename) << '_' << pid << '_' << stamp
       << ".html";
  const auto path = temp_dir / name.str();

  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return {};
  out.write(html.data(), static_cast<std::streamsize>(html.size()));
  out.close();
  if (!out) return {};

  const auto path_string = path.string();
  if (!open_path_in_browser(path_string)) return {};
  return path_string;
}

bool open_path_in_browser(std::string_view path_or_url) {
  const std::string target(path_or_url);
  if (target.empty()) return false;

#if defined(__APPLE__)
  const char* argv[] = {"open", target.c_str(), nullptr};
  pid_t child = 0;
  const int rc = ::posix_spawn(&child, "/usr/bin/open", nullptr, nullptr,
                                const_cast<char* const*>(argv), environ);
  return rc == 0;
#elif defined(__linux__)
  const char* argv[] = {"xdg-open", target.c_str(), nullptr};
  pid_t child = 0;
  // Rely on PATH resolution via posix_spawnp so this works whether
  // xdg-open lives in /usr/bin, /usr/local/bin, or elsewhere.
  const int rc = ::posix_spawnp(&child, "xdg-open", nullptr, nullptr,
                                 const_cast<char* const*>(argv), environ);
  return rc == 0;
#elif defined(_WIN32)
  const auto result = reinterpret_cast<INT_PTR>(::ShellExecuteA(
      nullptr, "open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
  return result > 32; // ShellExecute's own "success" threshold.
#else
  (void)target;
  return false;
#endif
}

} // namespace pluss::bi::detail
