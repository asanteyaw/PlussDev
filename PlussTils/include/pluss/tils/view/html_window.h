#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace pluss::view {

enum class BackendPreference : std::uint8_t {
  Auto = 0,
  NativeWebView = 1,
  PlussWebView = 2,
  PlussWebViewOnly = 3,
};

struct VirtualTableCell {
  std::string text{};
  bool missing{false};
};

class VirtualTableModel {
 public:
  virtual ~VirtualTableModel() = default;

  [[nodiscard]] virtual std::int64_t row_count() const noexcept = 0;
  [[nodiscard]] virtual std::int64_t column_count() const noexcept = 0;
  [[nodiscard]] virtual std::string row_header() const = 0;
  [[nodiscard]] virtual std::string row_label(std::int64_t row) const = 0;
  [[nodiscard]] virtual std::string column_name(std::int64_t column) const = 0;
  [[nodiscard]] virtual bool column_numeric(std::int64_t column) const noexcept = 0;
  [[nodiscard]] virtual VirtualTableCell cell(std::int64_t row,
                                              std::int64_t column) const = 0;
  [[nodiscard]] virtual bool row_matches(
      std::int64_t row,
      std::string_view folded_query) const = 0;
};

struct HtmlWindowOptions {
  std::string title{"PlussTilsView"};
  std::string snapshot_name{"view.png"};
  std::string svg_payload{};
  std::string svg_snapshot_name{"view.svg"};
  std::int64_t width{1280};
  std::int64_t height{760};
  BackendPreference backend_preference{BackendPreference::Auto};
  std::shared_ptr<const VirtualTableModel> virtual_table_model{};
  bool table_viewport_scroll_only{false};
};

[[nodiscard]] bool runtime_gui_environment_available() noexcept;
[[nodiscard]] bool native_backend_available() noexcept;
[[nodiscard]] bool pluss_webview_backend_available() noexcept;
[[nodiscard]] bool show_html_window(const std::string& html,
                                    const HtmlWindowOptions& options = {});

} // namespace pluss::view
