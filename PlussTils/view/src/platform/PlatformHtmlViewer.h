#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <string_view>
#include <vector>

#include "pluss/tils/view/html_window.h"

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
bool export_inhouse_table_xlsx(std::shared_ptr<const VirtualTableModel> model,
                               const std::string& path,
                               const std::vector<std::int64_t>* row_order = nullptr);
bool show_inhouse_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height,
                              const std::string& snapshot_name,
                              const std::string& svg_payload,
                              const std::string& svg_snapshot_name,
                              std::shared_ptr<const VirtualTableModel>
                                  virtual_table_model = {},
                              bool table_viewport_scroll_only = false,
                              std::vector<HtmlMenu> menus = {},
                              std::function<std::string(std::string_view)> render_panel = {},
                              std::function<std::string(std::string_view)> on_view = {},
                              std::function<std::string(std::string_view, const std::unordered_map<std::string, std::string>&)>
                                  on_action = {});

} // namespace pluss::view::detail
