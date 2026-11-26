#pragma once

#include <cstdint>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::table {

class Shelf;
class DataFrame;
class Atlas;

namespace display {

enum class DisplayMode : std::uint8_t {
  Auto = 0,
  Compact = 1,
  Wide = 2,
};

enum class DisplayBackend : std::uint8_t {
  Auto = 0,
  Terminal = 1,
  NativeWebView = 2,
  PlussWebView = 3,
};

// Colour theme of the HTML viewer (DataFrame / Shelf / Atlas windows).
enum class DisplayTheme : std::uint8_t {
  Auto = 0,       // use default_theme()
  Default = 1,    // cobalt blue and coral
  Ember = 2,      // warm clay and orange
  Nardo = 3,      // concrete grey and red
  Brooklands = 4, // racing green and gold
  Gulf = 5,       // powder blue and orange
  Obsidian = 6,   // charcoal and copper
  Indigo = 7,     // deep indigo and iris
  Native = 8,     // Apple's native web view palette; omitted from the Theme menu
};

// How numeric cells are written in the Table view (the stored values are unchanged).
enum class NumberFormat : std::uint8_t {
  Default = 0,     // the data as read, with no number formatting
  Decimals0 = 1,
  Decimals1 = 2,
  Decimals2 = 3,
  Decimals3 = 4,
  Decimals4 = 5,
  Thousands = 6,   // 1,234,567.89
  Scientific = 7,  // 1.235e+06
};

// One card of the keyed-cards view: a name, a few badges, and the values it holds.
struct CardPanel {
  std::string title;
  std::vector<std::string> badges;
  std::shared_ptr<const Shelf> values;
};

// Which rows the table shows, chosen from the View > Filter menu. Invalid is any cell that is NULL, NaN or
// infinite; the others pick one kind. The decision is made on what each cell is, never on its text.
enum class DisplayFilter : std::uint8_t {
  None = 0,
  Invalid = 1,
  Null = 2,
  NaN = 3,
  Infinite = 4,
};

// Which rows the table visits, chosen from View > Visit: the first, middle or last `visit_rows`, or a range of
// positions the user typed (Custom). The same rule applies to every table, long or short.
enum class DisplayVisit : std::uint8_t {
  All = 0,
  First = 1,
  Middle = 2,
  Last = 3,
  Custom = 4,
};

struct TerminalInfo {
  std::int64_t columns{120};
  std::int64_t rows{40};
  bool is_tty{false};
};

struct DisplayOptions {
  DisplayMode mode{DisplayMode::Auto};
  DisplayBackend backend{DisplayBackend::Auto};
  DisplayTheme theme{DisplayTheme::Auto};
  NumberFormat number_format{NumberFormat::Default};
  DisplayFilter filter{DisplayFilter::None};
  DisplayVisit visit{DisplayVisit::All};
  std::int64_t visit_rows{10};              // First, Middle and Last
  std::int64_t visit_begin{0};              // Custom: the first position, 0-based
  std::int64_t visit_end{std::numeric_limits<std::int64_t>::max()};  // Custom: the position to stop before
  std::int64_t terminal_width{0};  // <= 0 => detect from terminal
  std::int64_t terminal_height{0}; // <= 0 => detect from terminal
  bool show_shape_footer{true};
  bool show_case_index{true};
  bool color{false};
  std::int64_t viewer_width{1280};
  std::int64_t viewer_height{760};
  bool html_sortable{true};
  bool html_search{true};
  bool html_copy_button{true};
  bool html_desktop_table_viewport{false};
  bool html_summary{true};  // add a Summary tab (per-column statistics) next to the Table tab
  std::string window_title{};
};

TerminalInfo detect_terminal();
DisplayBackend default_backend();
void default_backend(DisplayBackend backend);
DisplayTheme default_theme();
void default_theme(DisplayTheme theme);
const char* theme_name(DisplayTheme theme);
bool parse_theme(const std::string& name, DisplayTheme& out);  // case-insensitive
bool gui_available();

std::string render(const Shelf& shelf, const DisplayOptions& options = {});
std::string render(const DataFrame& frame, const DisplayOptions& options = {});
std::string render(const Atlas& atlas, const DisplayOptions& options = {});
std::string render_info(const DataFrame& frame, const DisplayOptions& options = {});
std::string render_html(const Shelf& shelf, const DisplayOptions& options = {});
std::string render_html(const DataFrame& frame, const DisplayOptions& options = {});
std::string render_html(const Atlas& atlas, const DisplayOptions& options = {});
std::string info_html(const DataFrame& frame, const DisplayOptions& options = {});
// A page of side-by-side cards, one per named column of values; cards may differ in length.
std::string render_html_cards(std::string_view heading, std::string_view subtitle,
                              const std::vector<CardPanel>& cards, const DisplayOptions& options = {});

void print(std::ostream& os, const Shelf& shelf, const DisplayOptions& options = {});
void print(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});
void print(std::ostream& os, const Atlas& atlas, const DisplayOptions& options = {});
void print_info(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});
void show(const Shelf& shelf, const DisplayOptions& options = {});
void show(const DataFrame& frame, const DisplayOptions& options = {});
void show(const Atlas& atlas, const DisplayOptions& options = {});
void show_info(const DataFrame& frame, const DisplayOptions& options = {});
// Opens the cards page in the viewer; returns false when only a terminal is available.
bool show_cards(std::string_view heading, std::string_view subtitle,
                const std::vector<CardPanel>& cards, const DisplayOptions& options = {});

} // namespace display

} // namespace pluss::table
