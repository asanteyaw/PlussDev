// The View > Filter menu keeps rows by what a cell is (missing, NaN, infinite), never by its text.
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"
#include "kindle/Stream.h"
#include "kindle/display/GridModel.h"
#include "scribble/Slot.h"
#include "table/display/Display.h"
#include "table/display/RaggedGrid.h"
#include "table/display/StackPage.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Shelf;
namespace display = pluss::table::display;

void expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "[table_display_filter_tests] failure: " << message << "\n";
    std::exit(1);
  }
}

bool has(const std::string& page, std::string_view text) {
  return page.find(text) != std::string::npos;
}

// "Nancy" contains "nan" and "Ann" does not: the first must never be found as a NaN.
DataFrame sample() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  return DataFrame({
      {"name", Shelf(pluss::make_note({"Nancy", "Ann", "Bob", "Dee"}))},
      {"x", Shelf(pluss::make_series({1.0, nan, inf, 4.0}))},
      {"y", Shelf(pluss::make_series({1.0, 2.0, 3.0, 0.0}), pluss::make_series(std::vector<bool>{true, true, true, false}))},
  });
}

std::string page(const display::DisplayFilter filter) {
  display::DisplayOptions options;
  options.filter = filter;
  return display::render_html(sample(), options);
}

void test_no_filter_shows_every_row() {
  const auto all = page(display::DisplayFilter::None);
  expect(has(all, "Nancy") && has(all, "Ann") && has(all, "Bob") && has(all, "Dee"), "no filter keeps every row");
}

void test_invalid_keeps_null_nan_and_infinity() {
  const auto out = page(display::DisplayFilter::Invalid);
  expect(!has(out, "Nancy"), "a row with only ordinary values is dropped");
  expect(has(out, "Ann") && has(out, "Bob") && has(out, "Dee"), "NaN, infinity and NULL rows are kept");
  expect(has(out, "3 of 4 cases"), "the subtitle says how many rows are shown");
}

void test_each_kind_alone() {
  const auto nulls = page(display::DisplayFilter::Null);
  expect(has(nulls, "Dee") && !has(nulls, "Ann") && !has(nulls, "Bob") && !has(nulls, "Nancy"), "NULL keeps only the missing row");
  const auto nans = page(display::DisplayFilter::NaN);
  expect(has(nans, "Ann") && !has(nans, "Bob") && !has(nans, "Dee") && !has(nans, "Nancy"),
         "NaN keeps only the NaN row, and the name Nancy is not mistaken for one");
  const auto infs = page(display::DisplayFilter::Infinite);
  expect(has(infs, "Bob") && !has(infs, "Ann") && !has(infs, "Dee") && !has(infs, "Nancy"), "infinity keeps only the infinite row");
}

// The text of the table row that starts with `label`, cells joined by "|".
std::string row_text(const std::string& page, std::string_view label) {
  const auto at = page.find(label);
  if (at == std::string::npos) {
    return {};
  }
  const auto begin = page.rfind("<tr", at);
  const auto end = page.find("</tr>", at);
  std::string out;
  bool in_tag = false;
  for (std::size_t i = begin; i < end && i < page.size(); ++i) {
    if (page[i] == '<') {
      in_tag = true;
      out.push_back('|');
    } else if (page[i] == '>') {
      in_tag = false;
    } else if (!in_tag) {
      out.push_back(page[i]);
    }
  }
  return out;
}

// The Missing view counts each kind of cell per feature, and over the whole row in "(any column)".
void test_missing_view_counts() {
  const auto out = page(display::DisplayFilter::None);
  const auto any = row_text(out, "(any column)");
  expect(any.find("1||25||1||25||1||25") != std::string::npos || any.find("|1|25|1|25|1|25") != std::string::npos,
         "the (any column) row counts one NULL, one NaN and one infinity row (25 % each): " + any);
}

// A frame of n rows named row-01, row-02, ... so each can be looked for in the page.
DataFrame numbered(const int n) {
  std::vector<std::string> names;
  for (int i = 1; i <= n; ++i) {
    names.push_back(std::string("row-") + (i < 10 ? "0" : "") + std::to_string(i));
  }
  return DataFrame({{"name", Shelf(pluss::make_note(names))}});
}

// Which of row-01..row-n the Visit choice shows, as the numbers.
std::vector<int> visited(const int n, const display::DisplayVisit visit) {
  display::DisplayOptions options;
  options.visit = visit;
  const auto out = display::render_html(numbered(n), options);
  std::vector<int> shown;
  for (int i = 1; i <= n; ++i) {
    const std::string label = std::string("row-") + (i < 10 ? "0" : "") + std::to_string(i);
    // Count only the table (the page also carries the other views), so look for the label inside a cell.
    if (out.find(">" + label + "<") != std::string::npos) {
      shown.push_back(i);
    }
  }
  return shown;
}

std::vector<int> span(const int first, const int last) {
  std::vector<int> out;
  for (int i = first; i <= last; ++i) {
    out.push_back(i);
  }
  return out;
}

void test_visit_first_middle_last() {
  using Visit = display::DisplayVisit;
  expect(visited(5, Visit::First) == span(1, 5) && visited(5, Visit::Middle) == span(1, 5) && visited(5, Visit::Last) == span(1, 5),
         "with 5 rows every Visit choice shows all 5");
  expect(visited(11, Visit::First) == span(1, 10), "with 11 rows the first 10 are 1-10");
  expect(visited(11, Visit::Middle) == span(2, 11) && visited(11, Visit::Last) == span(2, 11),
         "with 11 rows the middle and the last 10 are both 2-11");
  expect(visited(12, Visit::First) == span(1, 10) && visited(12, Visit::Middle) == span(2, 11) && visited(12, Visit::Last) == span(3, 12),
         "with 12 rows the first, middle and last 10 differ: 1-10, 2-11, 3-12");
}

void test_visit_custom_range() {
  display::DisplayOptions options;
  options.visit = display::DisplayVisit::Custom;
  options.visit_begin = 3;
  options.visit_end = 7;
  const auto out = display::render_html(numbered(20), options);
  expect(out.find(">row-04<") != std::string::npos && out.find(">row-08<") != std::string::npos &&
             out.find(">row-03<") == std::string::npos && out.find(">row-09<") == std::string::npos,
         "a custom range of 3 to 7 shows the positions 3 through 7, both ends included");
  expect(out.find("visit-apply") == std::string::npos, "the range box is gone once the range is applied");
  options.visit_editing = true;
  expect(display::render_html(numbered(20), options).find("visit-apply") != std::string::npos,
         "the range box is on the page while it is being edited");
}

// A ragged catalog as a grid: each column as long as its stream, nothing padded, absent is not NULL.
void test_catalog_grid_is_ragged_not_padded() {
  using namespace pluss::kindle::literals;
  const double nan = std::numeric_limits<double>::quiet_NaN();
  auto catalog = pluss::kindle::Catalog({
      "names"_k = pluss::kindle::Stream(pluss::make_note({"John", "Pele", "Mensah"})),
      "scores"_k = pluss::kindle::Stream(pluss::make_series({1.5, nan, 3.5, 4.5, 5.5})),
      "flag"_k = pluss::kindle::Stream(pluss::make_series({1.0, 0.0}), pluss::make_series(std::vector<bool>{true, false})),
  });
  pluss::kindle::display::CatalogGridModel grid(catalog);
  expect(grid.row_count() == 5 && grid.column_count() == 3 && grid.longest() == 5 && grid.shortest() == 2,
         "the grid is as long as the longest key and as wide as the catalog");
  expect(grid.column_name(0) == "names \u00B7 3" && grid.column_name(1) == "scores \u00B7 5",
         "a header carries the key and how many entries it really has");
  expect(!grid.sortable(), "the independent columns of a ragged catalog are not sortable as rows");
  // Head: every key starts at the top.
  expect(grid.cell(0, 0).text == "John" && grid.cell(2, 0).text == "Mensah" && grid.cell(3, 0).absent && grid.cell(4, 0).absent,
         "a short key ends where its stream ends: the cells below are absent, not NULL");
  expect(!grid.cell(3, 0).missing, "an absent cell is not a missing one");
  expect(grid.cell(1, 2).missing && grid.cell(1, 2).text == "NULL" && !grid.cell(1, 2).absent,
         "a real NULL inside a stream is still a NULL");
  expect(grid.cell(1, 1).nonfinite && grid.cell(1, 1).text == "NAN", "a NaN inside a stream is shown as a NaN value");
  // Tail: every key ends at the bottom.
  grid.set_alignment(pluss::table::display::DisplayAlign::Tail);
  expect(grid.cell(0, 0).absent && grid.cell(1, 0).absent && grid.cell(2, 0).text == "John" && grid.cell(4, 0).text == "Mensah",
         "with tail alignment a short key sits at the bottom");
  grid.set_alignment(pluss::table::display::DisplayAlign::Head);
  // The filter keeps rows where some key holds that kind of entry, and absent cells never count.
  grid.set_filter(pluss::table::display::DisplayFilter::Null);
  expect(grid.filtering() && grid.row_allowed(1) && !grid.row_allowed(0) && !grid.row_allowed(4),
         "the NULL filter keeps the row with a NULL and none of the absent ones");
  grid.set_filter(pluss::table::display::DisplayFilter::NaN);
  expect(grid.row_allowed(1) && !grid.row_allowed(2), "the NaN filter keeps the row with the NaN");
  grid.set_filter(pluss::table::display::DisplayFilter::None);
  expect(!grid.filtering() && grid.row_allowed(4), "with no filter every row is kept");
}

// The Raggedness view says how ragged a catalog is, from the streams as they are.
void test_catalog_raggedness_view() {
  using namespace pluss::kindle::literals;
  auto catalog = pluss::kindle::Catalog({
      "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
      "b"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
      "c"_k = pluss::kindle::Stream(pluss::make_series({1.0, 0.0}), pluss::make_series(std::vector<bool>{true, false})),
  });
  display::ModelPage page;
  page.title = "Catalog";
  for (const auto& key : catalog.keys()) {
    page.streams.emplace_back(key, std::make_shared<const Shelf>(catalog.keys(key).to_shelf()));
  }
  const auto html = display::render_html_model(page);
  expect(has(html, ">Spread<") && has(html, ">Shortest<") && has(html, ">Full-length keys<"), "the raggedness cards are on the page");
  expect(has(html, ">1 of 3<"), "one of the three keys is full length");
  expect(has(html, "plussmic-ragged"), "the per-key raggedness table is on the page");
  // The row of key c: 2 entries, 3 absent, 40 % of the longest, 1 valid (one is NULL).
  const auto c_row = row_text(html.substr(html.find("plussmic-ragged")), ">c<");
  expect(c_row.find("2") != std::string::npos && c_row.find("3") != std::string::npos && c_row.find("40") != std::string::npos,
         "key c has 2 entries, lacks 3 and covers 40 % of the longest key: " + c_row);
}

}  // namespace

void test_notebook_table_is_a_plain_fragment() {
  std::vector<double> values;
  std::vector<bool> valid;
  for (int i = 0; i < 200; ++i) {
    values.push_back(i);
    valid.push_back(i != 3);
  }
  const auto frame = DataFrame({{"x", Shelf(pluss::make_series(values), pluss::make_series(valid))}});
  const auto html = pluss::table::display::render_notebook_html(frame);
  expect(!has(html, "<html") && !has(html, "<body") && !has(html, "<script"),
         "a notebook table is a fragment: no page and no scripts");
  expect(!has(html, "min-height") && !has(html, "100vh"), "a notebook table has no height of its own");
  expect(has(html, "class=\"pluss-table\""), "its styles are scoped to the table");
  expect(has(html, "class=\"null\">NULL"), "a missing value is marked");
  expect(has(html, "&hellip;"), "a long frame is cut to its first and last rows");
  expect(has(html, "200 cases"), "the footer says how many cases there are in all");
  expect(!has(html, "<td>100</td>"), "the cut rows are not written out");
}

// In a notebook a shelf is text, as in the terminal; a catalog is a ragged HTML grid, blank where a key has no entry.
void test_notebook_shows_each_type_in_its_own_shape() {
  setenv("PLUSS_NOTEBOOK", "1", 1);
  std::ostringstream captured;
  auto* const saved = std::cout.rdbuf(captured.rdbuf());
  const Shelf shelf(pluss::make_series({101.0, 102.0, 103.0}));
  pluss::table::display::show(shelf);
  const std::string shelf_text = captured.str();
  captured.str("");
  using namespace pluss::kindle::literals;
  auto catalog = pluss::kindle::Catalog({"a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
                                         "b"_k = pluss::kindle::Stream(pluss::make_series({4.0}))});
  catalog.show();
  const std::string catalog_text = captured.str();
  std::cout.rdbuf(saved);
  unsetenv("PLUSS_NOTEBOOK");
  expect(has(shelf_text, "101") && has(shelf_text, "103") && !has(shelf_text, "pluss-mime") && !has(shelf_text, "<table"),
         "a shelf prints its values as text");
  expect(has(catalog_text, "pluss-mime text/html") && has(catalog_text, "<table"), "a catalog is an HTML table");
  expect(has(catalog_text, "a &middot; 3") || has(catalog_text, "a \u00b7 3"), "whose headers say how long each key is");
  expect(has(catalog_text, "<td></td>"), "and whose missing entries are blank");
}

// A Shelf, a Stream and a Slot are one-dimensional and print alike, and printing costs the rows shown, not the length.
void test_one_dimensional_types_print_alike() {
  using Value = pluss::scribble::Slot::Value;
  const Shelf shelf(pluss::make_series({101.0, 102.0, 103.0}));
  const pluss::kindle::Stream stream(pluss::make_series({101.0, 102.0, 103.0}));
  std::ostringstream shelf_text, stream_text;
  pluss::table::display::print(shelf_text, shelf);
  stream_text << stream;
  expect(stream_text.str() == shelf_text.str(), "a stream prints exactly as the shelf of its values does");

  const pluss::scribble::Slot same(std::vector<Value>{Value(pluss::make_series({1.5})), Value(pluss::make_series({2.5}))});
  std::ostringstream same_text;
  same_text << same;
  expect(has(same_text.str(), "1.5") && has(same_text.str(), "ptype: Series") && !has(same_text.str(), "_type"),
         "a slot of one kind prints its values and says what it holds, not its internals");

  const pluss::scribble::Slot mixed(std::vector<Value>{Value(pluss::make_series({1.0})), Value(pluss::make_note({"a"})),
                                                       Value(std::monostate{})});
  std::ostringstream mixed_text;
  mixed_text << mixed;
  expect(has(mixed_text.str(), "NULL") && has(mixed_text.str(), "ptype: Mixed (Series, Note)"),
         "a slot of different kinds says so, and a cell with no value is NULL");

  // Only the rows that are shown are asked for, however long the sequence is.
  std::int64_t asked = 0;
  pluss::table::display::SequenceView view;
  view.size = 1'000'000;
  view.value = [&](const std::int64_t row) {
    ++asked;
    return std::to_string(row);
  };
  view.footer = "ptype: Series";
  const auto text = pluss::table::display::render_sequence(view);
  expect(has(text, "999999") && has(text, "..."), "a long sequence shows its first and last rows with dots between");
  expect(asked < 100, "and formats only those rows");
}

// A stack of tables (an atlas, an archive, an axel) says a short list of facts about itself, however deep it is and
// however many columns its tables have.
void test_stacks_print_a_summary_that_does_not_grow() {
  const auto build = [](const int frames_count, const int columns) {
    std::vector<DataFrame> frames;
    for (int f = 0; f < frames_count; ++f) {
      std::vector<std::pair<std::string, Shelf>> cols;
      for (int c = 0; c < columns; ++c) {
        cols.emplace_back("c" + std::to_string(c), Shelf(pluss::make_series({1.0 * f, 2.0})));
      }
      frames.emplace_back(cols);
    }
    return pluss::table::Atlas(frames);
  };
  const auto lines = [](const std::string& text) { return std::count(text.begin(), text.end(), '\n'); };
  std::ostringstream small, large;
  small << build(3, 4);
  large << build(150, 120);
  expect(has(large.str(), "150 frames") && has(large.str(), "120 each") && has(large.str(), "the same in every frame"),
         "an atlas says how deep it is, how big its frames are and that they share their names");
  expect(lines(large.str()) == lines(small.str()), "and prints as many lines for a deep, wide atlas as for a small one");
  expect(!has(large.str(), "c119"), "without listing its feature names");
}

// A stack's window paints one level at a time: choosing another level swaps the table and tells the window it changed.
void test_stack_page_switches_levels() {
  namespace display = pluss::table::display;
  display::StackSource source;
  source.title = "Archive";
  source.unit = "catalog";
  source.depth = 3;
  source.table = [](const std::int64_t level) -> std::shared_ptr<display::OptionsModel> {
    std::vector<display::RaggedColumn> columns;
    display::RaggedColumn column;
    column.name = "c";
    column.entries = 2 + level;  // a different length at each level
    column.text = [](const std::int64_t i) { return std::to_string(i); };
    columns.push_back(std::move(column));
    return std::make_shared<display::RaggedGridModel>("entry", std::move(columns));
  };
  const auto page = display::stack_model_page(std::move(source));
  expect(page.levels == 3 && page.model != nullptr, "a stack page knows how many levels it has");
  page.on_options(display::DisplayOptions{});
  expect(page.model->row_count() == 2, "it opens on the first level");
  const auto before = page.model->revision();
  display::DisplayOptions second;
  second.level = 2;
  page.on_options(second);
  expect(page.model->row_count() == 4, "choosing another level shows that level's table");
  expect(page.model->revision() != before, "and tells the window it is a different table");
  std::string subtitle;
  std::vector<std::pair<std::string, std::string>> cards;
  page.describe(second, subtitle, cards);
  expect(has(subtitle, "catalog 3 of 3"), "the page says which level it is showing");
  display::DisplayOptions beyond;
  beyond.level = 99;
  page.on_options(beyond);
  expect(page.model->row_count() == 4, "a level past the end is the last one");
}

int main() {
  test_no_filter_shows_every_row();
  test_invalid_keeps_null_nan_and_infinity();
  test_each_kind_alone();
  test_missing_view_counts();
  test_visit_first_middle_last();
  test_visit_custom_range();
  test_catalog_grid_is_ragged_not_padded();
  test_catalog_raggedness_view();
  test_notebook_table_is_a_plain_fragment();
  test_notebook_shows_each_type_in_its_own_shape();
  test_one_dimensional_types_print_alike();
  test_stacks_print_a_summary_that_does_not_grow();
  test_stack_page_switches_levels();
  std::cout << "[table_display_filter_tests] ok\n";
  return 0;
}
