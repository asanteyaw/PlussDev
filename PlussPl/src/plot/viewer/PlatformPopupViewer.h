#pragma once

#include <cstdint>
#include <string>

namespace pluss::plotism::detail {

bool mac_plot_viewer_available();
bool show_macos_plot_window(const std::string& title,
                            const std::string& svg_path,
                            std::int64_t width,
                            std::int64_t height);

} // namespace pluss::plotism::detail
