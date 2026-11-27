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

namespace pluss::kindle {
class Catalog;
class Archive;
}  // namespace pluss::kindle
namespace pluss::scribble {
class Registry;
class Axel;
}  // namespace pluss::scribble

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
  std::int64_t level{0};      // which level of a stack (an atlas, an archive, an axel) the window shows
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

// Paints the window's table page, as PlussView shows it, into a PNG file of the given size.
bool save_png(const DataFrame& frame, const std::string& path, const DisplayOptions& options = {}, int width = 1246,
              int height = 520);
void show(const Atlas& atlas, const DisplayOptions& options = {});
void show_info(const DataFrame& frame, const DisplayOptions& options = {});
// A page for any model-painted table that is not a DataFrame (a ragged catalog): the title, the cards above the
// table, the model, and what to do when a window menu changes the options (the filter, Visit, the alignment).
// A table model that takes the window's options (the filter, Visit, number format, alignment) as they change.
class OptionsModel : public pluss::view::VirtualTableModel {
 public:
  virtual void apply(const DisplayOptions& options) const = 0;
};

struct ModelPage {
  std::string title;
  std::string subtitle;
  std::vector<std::pair<std::string, std::string>> cards;  // label, value
  std::shared_ptr<pluss::view::VirtualTableModel> model;
  // The streams behind the grid, for the view tabs (Summary, Validity, Outliers, Correlation, Layout).
  std::vector<std::pair<std::string, std::shared_ptr<const Shelf>>> streams;
  std::string part_word{"key"};        // what a column is called in the view tabs: key, field
  std::string entries_word{"entries"};  // what its items are called: entries, states
  std::function<void(const DisplayOptions&)> on_options;
  bool offer_align{false};  // View > Align (head, tail), for keys of different lengths
  // A stack's page: how many levels there are (a Level menu moves between them), what a level is called, and what its
  // page says (the subtitle and the cards) when `options.level` is shown. `model` is then the stack's switchable model.
  std::int64_t levels{1};
  std::function<std::string(std::int64_t)> level_label;
  std::function<void(const DisplayOptions&, std::string& subtitle, std::vector<std::pair<std::string, std::string>>& cards)> describe;
};
// The same for a model page (a ragged catalog): the page's own filter, visit and alignment come from the options.
bool save_png(const ModelPage& page, const std::string& path, const DisplayOptions& options = {}, int width = 1300,
              int height = 520, const std::string& view = {});
// A number's text written as View > Format asks for (the stored value does not change).
std::string format_number(const std::string& text, NumberFormat format);
// The row window View > Visit asks for, as the viewer's own type.
pluss::view::RowWindow row_window_for(const DisplayOptions& options);
// One-dimensional text, the layout a Shelf prints: the position and the value of each cell, the first and last rows
// with a row of dots between when there are many, then a footer ("ptype: Series"). A Shelf, a Stream and a Slot all
// print through it, so they look alike in a terminal and in a notebook. Only the rows that are shown are asked for
// (`label` and `value` are called for those alone), so the cost does not grow with the length of the sequence.
struct SequenceView {
  std::int64_t size{0};
  std::function<std::string(std::int64_t)> label;  // the text of a row's label; unset: the row's position
  std::function<std::string(std::int64_t)> value;  // the text of a row's value
  std::string footer;                              // the line under the rows, such as "ptype: Series"
};
std::string render_sequence(const SequenceView& view, const DisplayOptions& options = {});
void print_sequence(std::ostream& os, const SequenceView& view, const DisplayOptions& options = {});
// The overview of a collection of named parts (a registry's fields, an axel's levels): a title, optional note lines and
// one row per part under named columns, the last column taking the rest of the line. Only the rows shown are asked for.
struct SummaryView {
  std::string title;                 // the first line, such as "scribble::Registry  fields=2  states=3"
  std::vector<std::string> notes;    // lines between the title and the rows
  std::vector<std::string> columns;  // the column titles
  std::int64_t size{0};              // how many rows there are in all
  std::function<std::vector<std::string>(std::int64_t)> row;  // the cells of one row, one per column
};
std::string render_summary(const SummaryView& view, const DisplayOptions& options = {});
void print_summary(std::ostream& os, const SummaryView& view, const DisplayOptions& options = {});
// "[a, b, c, ..., z]": the first and last values of a sequence on one line, for a column that previews a part.
std::string preview_text(std::int64_t size, const std::function<std::string(std::int64_t)>& value, std::int64_t max_width);

// What a stack of tables (an atlas, an archive, an axel) says about itself in a terminal or a notebook: a short list of
// facts, the same length however deep the stack is or however many columns its tables have. The tables themselves, one
// per level, are for the PlussView window.
struct StackFacts {
  std::string type;                                      // "Atlas", "Archive", "Axel"
  std::vector<std::pair<std::string, std::string>> facts;  // name, value
};
std::string render_stack_facts(const StackFacts& stack, const DisplayOptions& options = {});
void print_stack_facts(std::ostream& os, const StackFacts& stack, const DisplayOptions& options = {});
std::string render_notebook_stack_facts(const StackFacts& stack);
// "5 each" when every level has the same count, else "2 to 120 (1234 in all)"; "none" with no levels.
std::string span_text(std::int64_t smallest, std::int64_t largest, std::int64_t total, std::int64_t count);
// "Series × 12, Note × 3", the most common kinds first, the rest counted rather than listed.
std::string tally_text(const std::vector<std::pair<std::string, std::int64_t>>& counts);

// The text of a Shelf's first cell, as the Shelf prints it (a Slot cell is a one-element Shelf).
std::string first_cell_text(const Shelf& shelf);

// The compact HTML table of any model PlussView can paint (a catalog's ragged grid, a registry's), blank where a column
// has no entry, with `footer` under it; and the same for an overview (the first column labels the rows).
std::string render_notebook_html(const pluss::view::VirtualTableModel& model, const std::string& footer);
std::string render_notebook_summary(const SummaryView& view, const std::string& footer);
// Hands `html` to the notebook as the output of the cell; false (and nothing printed) outside the notebook.
bool show_in_notebook(const std::string& html);
// The notebook table of a catalog, an archive, a registry and an axel, and of an atlas (defined with each type).
std::string render_notebook_html(const pluss::kindle::Catalog& catalog);
std::string render_notebook_html(const pluss::kindle::Archive& archive);
std::string render_notebook_html(const pluss::scribble::Registry& registry);
std::string render_notebook_html(const pluss::scribble::Axel& axel);
std::string render_notebook_html(const Atlas& atlas);
// Whether the code is running inside the PlussII notebook (its kernel sets PLUSS_NOTEBOOK). Only a DataFrame is shown
// there as a table; a shelf, an atlas and a catalog are shown as the text the terminal shows.
bool in_notebook();
// The frame as the compact HTML table a notebook shows: no page and no scripts, styles scoped to the table, the
// notebook's own colours, as tall as its rows (a long frame as its first and last rows).
std::string render_notebook_html(const DataFrame& frame);
// A plot's SVG for a notebook: the picture itself (the editor's own Save and plot viewer apply to it), as wide as the
// output and as tall as that width needs, never wider than it was drawn.
std::string render_notebook_plot_svg(std::string_view svg);
// The page as HTML with every view tab filled in (for a browser or a notebook, and for tests).
std::string render_html_model(const ModelPage& page, const DisplayOptions& options = {});
// Opens the page in the viewer; returns false when only a terminal is available.
bool show_model(const ModelPage& page, const DisplayOptions& options = {});


} // namespace display

} // namespace pluss::table
