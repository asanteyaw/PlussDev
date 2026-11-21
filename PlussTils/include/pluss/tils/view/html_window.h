#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::view {

enum class BackendPreference : std::uint8_t {
  Auto = 0,
  NativeWebView = 1,
  PlussWebView = 2,
  PlussWebViewOnly = 3,
};

struct VirtualTableCell {
  std::string text{};
  bool missing{false};    // a NULL cell (no value): drawn in red
  bool nonfinite{false};  // NaN / inf / -inf: a value, drawn in a neutral tint, never as missing
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

// A window-level menu (e.g. colour theme, number format). Choosing an option calls
// `render(option id)`, which returns the full page re-rendered for that choice.
struct HtmlMenuOption {
  std::string id{};
  std::string label{};
};

struct HtmlMenu {
  std::string id{};
  std::string title{};
  std::vector<HtmlMenuOption> options{};
  std::function<std::string(std::string_view option_id)> render{};
  // When true the menu is not a toolbar pull-down of its own: it appears as a cascading
  // entry (with an arrow) inside the "View" menu.
  bool in_view_menu{false};
  // With in_view_menu: groups the cascading entry under a named entry of the View menu (several menus with the
  // same section share one), so a long list of menus stays browsable. Empty: the cascade sits in View itself.
  std::string section{};
  // Returns the id of the option currently in effect, which the menu shows with a check mark; empty means the
  // first option. Called after every render. Optional.
  std::function<std::string()> selected{};
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
  std::vector<HtmlMenu> menus{};
  // Fills an empty view panel (an element with data-lazy="<id>") the first time it is shown:
  // returns the panel's inner HTML for that id. Lets expensive views be computed on demand.
  std::function<std::string(std::string_view id)> render_panel{};
};

[[nodiscard]] bool runtime_gui_environment_available() noexcept;
[[nodiscard]] bool native_backend_available() noexcept;
[[nodiscard]] bool pluss_webview_backend_available() noexcept;
[[nodiscard]] bool show_html_window(const std::string& html,
                                    const HtmlWindowOptions& options = {});

} // namespace pluss::view
