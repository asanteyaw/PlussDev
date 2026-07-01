#include "pluss/tils/view/window/shell.h"

#include <cstdlib>
#include <string_view>
#include <memory>

namespace pluss::view::window {

#if defined(__APPLE__)
[[nodiscard]] std::unique_ptr<IWindowShell> make_cocoa_window_shell();
#elif defined(_WIN32)
[[nodiscard]] std::unique_ptr<IWindowShell> make_win32_window_shell();
#elif defined(PLUSSTILS_VIEW_HAVE_X11_SHELL) && PLUSSTILS_VIEW_HAVE_X11_SHELL
[[nodiscard]] std::unique_ptr<IWindowShell> make_x11_window_shell();
#endif

namespace {
[[nodiscard]] std::unique_ptr<IWindowShell> make_platform_window_shell();

[[nodiscard]] bool env_requests_native_shell() {
  const char* raw = std::getenv("PLUSSTILS_VIEW_WINDOW_NATIVE");
  if (raw == nullptr) {
    return false;
  }
  const std::string_view value(raw);
  return value == "1" || value == "true" || value == "TRUE" ||
         value == "yes" || value == "on";
}
} // namespace

std::unique_ptr<IWindowShell> make_window_shell(const ShellSelectionPolicy policy) {
  switch (policy) {
    case ShellSelectionPolicy::PortableOnly:
      return make_null_window_shell();
    case ShellSelectionPolicy::PortablePreferred:
      if (env_requests_native_shell()) {
        if (auto shell = make_platform_window_shell()) {
          return shell;
        }
      }
      return make_null_window_shell();
    case ShellSelectionPolicy::NativePreferred:
      if (auto shell = make_platform_window_shell()) {
        return shell;
      }
      return make_null_window_shell();
    case ShellSelectionPolicy::NativeOnly:
      return make_platform_window_shell();
  }
  return make_null_window_shell();
}

namespace {

#if defined(__APPLE__)
std::unique_ptr<IWindowShell> make_platform_window_shell() {
  return make_cocoa_window_shell();
}
#elif defined(_WIN32)
std::unique_ptr<IWindowShell> make_platform_window_shell() {
  return make_win32_window_shell();
}
#elif defined(PLUSSTILS_VIEW_HAVE_X11_SHELL) && PLUSSTILS_VIEW_HAVE_X11_SHELL
std::unique_ptr<IWindowShell> make_platform_window_shell() {
  const char* display = std::getenv("DISPLAY");
  if (display == nullptr || display[0] == '\0') {
    return {};
  }
  return make_x11_window_shell();
}
#else
std::unique_ptr<IWindowShell> make_platform_window_shell() { return {}; }
#endif

} // namespace

} // namespace pluss::view::window
