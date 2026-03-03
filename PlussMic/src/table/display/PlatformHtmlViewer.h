#pragma once

#include <cstdint>
#include <string>

namespace pluss::table::display::detail {

bool mac_html_viewer_available();
bool show_macos_html_window(const std::string& title,
                            const std::string& html,
                            std::int64_t width,
                            std::int64_t height);
bool windows_html_viewer_available();
bool show_windows_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height);

} // namespace pluss::table::display::detail
