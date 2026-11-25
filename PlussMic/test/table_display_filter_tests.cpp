// The View > Filter menu keeps rows by what a cell is (missing, NaN, infinite), never by its text.
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"
#include "table/display/Display.h"

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

}  // namespace

int main() {
  test_no_filter_shows_every_row();
  test_invalid_keeps_null_nan_and_infinity();
  test_each_kind_alone();
  test_missing_view_counts();
  std::cout << "[table_display_filter_tests] ok\n";
  return 0;
}
