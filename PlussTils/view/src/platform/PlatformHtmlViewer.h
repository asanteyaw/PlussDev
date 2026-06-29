#pragma once

#include <cstdint>
#include <string>

namespace pluss::view::detail {

bool gtk_html_viewer_available();
bool show_gtk_html_window(const std::string& title,
                          const std::string& html,
                          std::int64_t width,
                          std::int64_t height,
                          const std::string& snapshot_name,
                          const std::string& svg_payload,
                          const std::string& svg_snapshot_name);
bool mac_html_viewer_available();
bool show_macos_html_window(const std::string& title,
                            const std::string& html,
                            std::int64_t width,
                            std::int64_t height,
                            const std::string& snapshot_name);
bool windows_html_viewer_available();
bool show_windows_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height,
                              const std::string& snapshot_name);
bool inhouse_html_viewer_available();
bool export_inhouse_table_xlsx(const std::string& html, const std::string& path);
bool show_inhouse_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height,
                              const std::string& snapshot_name,
                              const std::string& svg_payload,
                              const std::string& svg_snapshot_name);

} // namespace pluss::view::detail
