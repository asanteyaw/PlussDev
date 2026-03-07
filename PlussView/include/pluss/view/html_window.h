#pragma once

#include <cstdint>
#include <string>

namespace pluss::view {

enum class BackendPreference : std::uint8_t {
  Auto = 0,
  NativeWebView = 1,
  PlussWebView = 2,
  PlussWebViewOnly = 3,
};

struct HtmlWindowOptions {
  std::string title{"PlussView"};
  std::string snapshot_name{"view.png"};
  std::string svg_payload{};
  std::string svg_snapshot_name{"view.svg"};
  std::int64_t width{1280};
  std::int64_t height{760};
  BackendPreference backend_preference{BackendPreference::Auto};
};

[[nodiscard]] bool runtime_gui_environment_available() noexcept;
[[nodiscard]] bool native_backend_available() noexcept;
[[nodiscard]] bool show_html_window(const std::string& html,
                                    const HtmlWindowOptions& options = {});

} // namespace pluss::view
