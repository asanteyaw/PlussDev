#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/tils/view/html_window.h"

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
  Paperwhite = 9, // an e-reader's white page: serif type, hairline rules, no stripes
  Sepia = 10,     // an e-reader's warm page
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

// Where the entries of keys of different lengths sit against each other in a ragged grid.
enum class DisplayAlign : std::uint8_t {
  Head = 0,  // every key starts at the top
  Tail = 1,  // every key ends at the bottom
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
  DisplayAlign align{DisplayAlign::Head};
  std::int64_t visit_rows{10};              // First, Middle and Last
  std::int64_t visit_begin{0};              // Custom: the first position, 0-based
  std::int64_t visit_end{std::numeric_limits<std::int64_t>::max()};  // Custom: the position to stop before
  bool visit_editing{false};  // the box for the custom range is open in the header, waiting for Apply
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

void print(std::ostream& os, const Shelf& shelf, const DisplayOptions& options = {});
void print(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});
void print(std::ostream& os, const Atlas& atlas, const DisplayOptions& options = {});
void print_info(std::ostream& os, const DataFrame& frame, const DisplayOptions& options = {});
void show(const Shelf& shelf, const DisplayOptions& options = {});
void show(const DataFrame& frame, const DisplayOptions& options = {});
void show(const Atlas& atlas, const DisplayOptions& options = {});
void show_info(const DataFrame& frame, const DisplayOptions& options = {});
// A page for any model-painted table that is not a DataFrame (a ragged catalog): the title, the cards above the
// table, the model, and what to do when a window menu changes the options (the filter, Visit, the alignment).
struct ModelPage {
  std::string title;
  std::string subtitle;
  std::vector<std::pair<std::string, std::string>> cards;  // label, value
  std::shared_ptr<pluss::view::VirtualTableModel> model;
  std::function<void(const DisplayOptions&)> on_options;
  bool offer_align{false};  // View > Align (head, tail), for keys of different lengths
};
// The row window View > Visit asks for, as the viewer's own type.
pluss::view::RowWindow row_window_for(const DisplayOptions& options);
// Opens the page in the viewer; returns false when only a terminal is available.
bool show_model(const ModelPage& page, const DisplayOptions& options = {});


} // namespace display

} // namespace pluss::table
