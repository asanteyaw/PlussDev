#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
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
  bool missing{false};    // a NULL cell (no value): its text is drawn in the theme's missing colour
  bool nonfinite{false};  // NaN / inf / -inf: a value, drawn like any other
  bool infinite{false};   // with `nonfinite`: inf / -inf rather than NaN
  // No entry here at all: a ragged column has ended, so nothing is drawn (no fill, no lines, no text). Not the
  // same as `missing`, which is an entry whose value is unknown.
  bool absent{false};
};

// Which rows of a table are visited: all of them, the first n, the middle n, the last n, or a position range.
// Positions are 0-based, as in the table's case column, and a range includes both of its ends: 3 to 8 is the rows
// 3, 4, 5, 6, 7 and 8. A negative position counts from the back (-1 is the last row).
struct RowWindow {
  enum class Kind : std::uint8_t { All = 0, First = 1, Middle = 2, Last = 3, Range = 4 };
  Kind kind{Kind::All};
  std::int64_t count{0};  // First, Middle and Last: how many rows
  std::int64_t begin{0};  // Range
  std::int64_t end{0};    // Range
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
  // A row filter chosen from a window menu. `filtering()` says whether one is in force; `row_allowed` is
  // then the test every row must pass, on top of any search text.
  [[nodiscard]] virtual bool filtering() const noexcept { return false; }
  [[nodiscard]] virtual bool row_allowed(std::int64_t /*row*/) const { return true; }
  // The rows to show out of those the filter, the search and the sort leave (see sort_model_rows and
  // window_bounds in table_order.h).
  [[nodiscard]] virtual RowWindow row_window() const { return {}; }
  // False for a table whose rows are not one thing to reorder (a ragged catalog's columns are independent), so its
  // headers carry no sort arrows.
  [[nodiscard]] virtual bool sortable() const noexcept { return true; }
  // A fraction in [0, 1] drawn as a thin progress bar under a column's name (how much of the longest column this one
  // covers, say), or a negative number for no bar.
  [[nodiscard]] virtual double column_meter(std::int64_t /*column*/) const { return -1.0; }
};

// A window-level menu (e.g. colour theme, number format). Choosing an option calls
// `render(option id)`, which returns the full page re-rendered for that choice.
// A menu option that needs values before it can act: choosing it shows a dialog with these fields (when the
// window can) and passes what was typed to HtmlMenu::render_prompted.
struct HtmlPromptField {
  std::string label{};
  std::string placeholder{};
};

struct HtmlMenuOption {
  std::string id{};
  std::string label{};
  std::string prompt_title{};  // empty: the option acts at once
  std::string prompt_message{};
  std::string prompt_accept{"OK"};
  std::vector<HtmlPromptField> prompt{};
};

struct HtmlMenu {
  std::string id{};
  std::string title{};
  std::vector<HtmlMenuOption> options{};
  std::function<std::string(std::string_view option_id)> render{};
  // For options with a prompt: the values the fields start with, and the page for what was typed. When the window
  // cannot show a dialog the option falls back to `render`, which can ask in the page.
  std::function<std::vector<std::string>(std::string_view option_id)> prompt_values{};
  std::function<std::string(std::string_view option_id, const std::vector<std::string>& values)> render_prompted{};
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
  // Called whenever a view tab is chosen (even the one already showing) with that tab's button id. Returns the
  // full page to show in its place, or empty to leave the page as it is. Lets a window reset what a menu set,
  // such as a row filter, when the user goes back to the plain view.
  std::function<std::string(std::string_view button_id)> on_view{};
  // Called when a button the page itself defines is clicked, with its id and the text typed into the page's input
  // boxes (by id). Returns the full page to show in its place, or empty to leave it. Lets a menu choice ask for
  // values, such as the first and last row to visit.
  std::function<std::string(std::string_view button_id,
                            const std::unordered_map<std::string, std::string>& inputs)>
      on_action{};
};

[[nodiscard]] bool runtime_gui_environment_available() noexcept;
[[nodiscard]] bool native_backend_available() noexcept;
[[nodiscard]] bool pluss_webview_backend_available() noexcept;
[[nodiscard]] bool show_html_window(const std::string& html,
                                    const HtmlWindowOptions& options = {});

} // namespace pluss::view
