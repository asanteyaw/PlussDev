#include "pluss/view/html_window.h"

#include <cstdlib>

#include "pluss/view/session.h"

namespace pluss::view {

bool runtime_gui_environment_available() noexcept {
#if defined(_WIN32) || defined(__APPLE__)
  return true;
#else
  return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#endif
}

bool native_backend_available() noexcept {
  return native_host_capabilities().available;
}

bool show_html_window(const std::string& html, const HtmlWindowOptions& options) {
  if (html.empty()) {
    return false;
  }
  auto session = make_session(options);
  return session.open(html);
}

} // namespace pluss::view
