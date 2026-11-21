#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/mic.h"
#include "pluss/tils/view/window.h"
#include "table/display/Display.h"

namespace {

void require(const std::string& html, std::string_view fragment, bool present = true) {
  if ((html.find(fragment) != std::string::npos) != present) {
    throw std::runtime_error(std::string("DataFrame view ") + (present ? "is missing " : "unexpectedly has ") +
                             std::string(fragment));
  }
}

} // namespace

int main() {
  try {
    using pluss::table::DataFrame;
    using pluss::table::Shelf;

    const DataFrame frame({{"load", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
                           {"speed", Shelf(pluss::make_series(std::vector<double>{2.0, 1.0, 4.0, 3.5}))},
                           {"city", Shelf(pluss::make_note({"A", "B", "C", "D"}))}});

    const auto html = pluss::table::display::render_html(frame);
    require(html, "id=\"view-toggle\"");
    require(html, "id=\"view-menu\"");
    require(html, "id=\"tab-table\"");
    require(html, "id=\"tab-summary\"");
    require(html, "id=\"panel-summary\"");
    require(html, "class=\"tab-panel active\" data-tab-group=\"main\"");
    require(html, "id=\"plussmic-summary\"");
    require(html, ">mean<");
    require(html, ">count<");
    require(html, ">max<");
    require(html, ">ptype<", false);         // always Series for numeric columns, so not shown
    require(html, ">top<", false);          // the Summary is numeric-only: no top-value column
    require(html, "id=\"plussmic-missing\"");
    require(html, ">null<");
    require(html, ">null %<");
    require(html, ">NaN<");
    require(html, ">NaN %<");
    require(html, ">infinity<");
    require(html, ">infinity %<");
    require(html, ">Validity<");
    require(html, ">present<", false);
    require(html, ">longest gap<", false);
    require(html, "first gap at", false);
    require(html, "id=\"tab-duplicates\"", false);  // no Duplicates view
    require(html, "id=\"plussmic-duplicates\"", false);
    require(html, "id=\"tab-correlation\"");
    for (const auto* view : {"missing", "outliers"}) {
      require(html, std::string("id=\"tab-") + view + "\"");
      require(html, std::string("id=\"panel-") + view + "\"");
    }
    require(html, "id=\"tab-distributions\"", false);
    require(html, "\u2581", false);  // no sparkline plots anywhere
    require(html, "id=\"plussmic-missing\"");
    require(html, "id=\"plussmic-corr\"");
    require(html, "background-color: rgba(");

    pluss::table::display::DisplayOptions off;
    off.html_summary = false;
    require(pluss::table::display::render_html(frame, off), "id=\"tab-summary\"", false);

    // Text-only frames get neither Summary nor Correlation (both are numeric), but still
    // get Missing.
    const DataFrame text_only({{"city", Shelf(pluss::make_note({"A", "B"}))}});
    const auto text_html = pluss::table::display::render_html(text_only);
    require(text_html, "id=\"tab-summary\"", false);
    require(text_html, "id=\"tab-correlation\"", false);
    require(text_html, "id=\"tab-missing\"");

    const DataFrame validity({{"reading", Shelf(pluss::make_series(std::vector<double>{
                                              1.0, std::numeric_limits<double>::quiet_NaN(),
                                              std::numeric_limits<double>::infinity(), 4.0}),
                                          pluss::make_series(std::vector<bool>{true, true, true, false}))}});
    const auto validity_html = pluss::table::display::render_html(validity);
    const auto validity_start = validity_html.find("id=\"plussmic-missing\"");
    const auto validity_end = validity_html.find("</table>", validity_start);
    if (validity_start == std::string::npos || validity_end == std::string::npos) {
      throw std::runtime_error("Validity view is missing");
    }
    const auto validity_view = validity_html.substr(validity_start, validity_end - validity_start);
    require(validity_view,
            ">reading</td><td class=\"num\">1</td><td class=\"num\">25</td><td class=\"num\">1</td>"
            "<td class=\"num\">25</td><td class=\"num\">1</td><td class=\"num\">25</td>");

    // Number format rewrites the data table's cells only.
    const DataFrame wide({{"big", Shelf(pluss::make_series(std::vector<double>{1234567.891, 2.5}))}});
    pluss::table::display::DisplayOptions fmt;
    fmt.number_format = pluss::table::display::NumberFormat::Decimals2;
    require(pluss::table::display::render_html(wide, fmt), ">1234567.89<");
    fmt.number_format = pluss::table::display::NumberFormat::Thousands;
    require(pluss::table::display::render_html(wide, fmt), ">1,234,567.89<");
    fmt.number_format = pluss::table::display::NumberFormat::Scientific;
    require(pluss::table::display::render_html(wide, fmt), ">1.235e+06<");
    require(pluss::table::display::render_html(wide), ">1234567.89<", false);  // Default leaves values as read

    // The format reaches every numeric column, integers included ...
    const DataFrame ints({{"n", Shelf(pluss::make_series(std::vector<std::int64_t>{1234567, 2}))}});
    fmt.number_format = pluss::table::display::NumberFormat::Decimals2;
    require(pluss::table::display::render_html(ints, fmt), ">1234567.00<");
    fmt.number_format = pluss::table::display::NumberFormat::Thousands;
    require(pluss::table::display::render_html(ints, fmt), ">1,234,567.00<");

    // ... and every view, not just the Table: the Summary's mean (617285.1955) follows it.
    require(pluss::table::display::render_html(wide), ">617285.1955<");
    fmt.number_format = pluss::table::display::NumberFormat::Decimals2;
    require(pluss::table::display::render_html(wide, fmt), ">617285.20<");
    fmt.number_format = pluss::table::display::NumberFormat::Thousands;
    require(pluss::table::display::render_html(wide, fmt), ">617,285.20<");

    // Wide frames: the correlation view lists the strongest pairs instead of an N x N matrix.
    std::vector<std::pair<std::string, Shelf>> wide_columns;
    for (int k = 0; k < 30; ++k) {
      std::vector<double> col;
      for (int i = 0; i < 8; ++i) {
        col.push_back(static_cast<double>((i * (k % 5 + 1) + k * i * i) % 11));
      }
      wide_columns.emplace_back("c" + std::to_string(k), Shelf(pluss::make_series(col)));
    }
    const auto wide_html = pluss::table::display::render_html(DataFrame(std::move(wide_columns)));
    require(wide_html, "id=\"plussmic-corr\"");
    require(wide_html, ">rank<");
    require(wide_html, ">with<");
    require(wide_html, "strongest pairs");
    require(html, ">rank<", false);  // small frames keep the full matrix

    // Long frames in a browser view draw their rows from data (only a window exists in the page);
    // short frames and the in-house view keep ordinary rows.
    {
      std::vector<double> long_values;
      for (int i = 0; i < 2500; ++i) {
        long_values.push_back(i * 0.5);
      }
      const DataFrame long_frame({{"v", Shelf(pluss::make_series(long_values))}});
      pluss::table::display::DisplayOptions native_view;
      native_view.backend = pluss::table::display::DisplayBackend::NativeWebView;
      const auto virtual_html = pluss::table::display::render_html(long_frame, native_view);
      require(virtual_html, "data-virtual=\"true\"");
      require(virtual_html, "application/json");
      require(virtual_html, "id=\"plussmic-table-data\"");
      require(virtual_html, "id=\"plussmic-table-badge\"");
      require(pluss::table::display::render_html(long_frame), "application/json", false);  // in-house view
      require(pluss::table::display::render_html(frame, native_view), "application/json", false);  // short frame
    }

    // Every view carries its own Cases / Features / Case Axis cards (not only the Table view):
    // 5 views x 3 cards. Summary's cards describe the summary table (one row per feature).
    {
      std::size_t cards = 0;
      for (auto at = html.find("class=\"summary-card\""); at != std::string::npos;
           at = html.find("class=\"summary-card\"", at + 1)) {
        ++cards;
      }
      if (cards < 15) {
        throw std::runtime_error("expected the summary cards in every view, found " + std::to_string(cards));
      }
    }

    // A browser view gives every column (the label column included) one fixed width and lets
    // long names wrap at their separators; the in-house view sizes its own columns.
    {
      const DataFrame named({{"customer_lifetime_value", Shelf(pluss::make_series(std::vector<double>{1.0, 2.0}))},
                             {"x", Shelf(pluss::make_series(std::vector<double>{3.0, 4.0}))}});
      pluss::table::display::DisplayOptions browser;
      browser.backend = pluss::table::display::DisplayBackend::NativeWebView;
      const auto browser_html = pluss::table::display::render_html(named, browser);
      require(browser_html, "customer_<wbr>lifetime_<wbr>value");
      require(browser_html, "<col style=\"width:150px\">");
      require(browser_html, "style=\"width:450px\"");  // 3 columns x 150px
      const auto inhouse_html = pluss::table::display::render_html(named);
      require(inhouse_html, "<wbr>", false);
      require(inhouse_html, "<col style=\"width:150px\">", false);
    }

    // The page carries browser-only CSS (attribute selectors); the in-house layout engine must
    // lay it out and finish (an unsupported selector once made it loop forever).
    {
      auto document = pluss::view::window::parse_html_document(pluss::table::display::render_html(frame));
      const auto scene = pluss::view::window::build_html_document_scene(
          document, pluss::view::window::SizeI{1000, 600}, pluss::view::window::HtmlRenderConfig{});
      if (scene.commands().empty()) {
        throw std::runtime_error("in-house layout produced an empty scene");
      }
    }

    // Outliers: fences, counts, share and the most extreme value per numeric column.
    const DataFrame spiky({{"v", Shelf(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 100}))}});
    const auto spiky_html = pluss::table::display::render_html(spiky);
    for (const auto* header : {">floor<", ">ceiling<", ">below<", ">above<", ">percent<", ">extreme<"}) {
      require(spiky_html, header);
    }
    require(spiky_html, "id=\"plussmic-outliers\"");
    require(spiky_html, ">lower fence<", false);  // renamed to one-word headers

    // Every theme must define all 29 colour tokens and tint the correlation matrix.
    for (const auto theme : {pluss::table::display::DisplayTheme::Default,
                             pluss::table::display::DisplayTheme::Ember,
                             pluss::table::display::DisplayTheme::Nardo,
                             pluss::table::display::DisplayTheme::Brooklands,
                             pluss::table::display::DisplayTheme::Gulf,
                             pluss::table::display::DisplayTheme::Obsidian,
                             pluss::table::display::DisplayTheme::Indigo,
                             pluss::table::display::DisplayTheme::Native}) {
      pluss::table::display::DisplayOptions themed;
      themed.theme = theme;
      const auto themed_html = pluss::table::display::render_html(frame, themed);
      const auto begin = themed_html.find(":root {");
      const auto end = themed_html.find("}", begin);
      if (begin == std::string::npos || end == std::string::npos) {
        throw std::runtime_error("theme root block missing");
      }
      const auto root = themed_html.substr(begin, end - begin);
      std::size_t tokens = 0;
      for (std::size_t at = root.find("--"); at != std::string::npos; at = root.find("--", at + 2)) {
        ++tokens;
      }
      if (tokens != 30 /* 29 colours + --shadow */ || root.find(": ;") != std::string::npos) {
        throw std::runtime_error(std::string("theme ") + pluss::table::display::theme_name(theme) +
                                 " has an incomplete palette");
      }
      require(themed_html, "background-color: rgba(");
    }
    // Every backend starts with the same selected default palette.
    {
      pluss::table::display::DisplayOptions native_opts;
      native_opts.backend = pluss::table::display::DisplayBackend::NativeWebView;
      require(pluss::table::display::render_html(frame, native_opts), "--paper: #f3f6fb");
      native_opts.theme = pluss::table::display::DisplayTheme::Native;
      require(pluss::table::display::render_html(frame, native_opts), "--paper: #e1e9f8");
      require(pluss::table::display::render_html(frame), "--paper: #f3f6fb");
    }
    pluss::table::display::DisplayTheme parsed{};
    if (!pluss::table::display::parse_theme("Brooklands", parsed) ||
        parsed != pluss::table::display::DisplayTheme::Brooklands ||
        pluss::table::display::parse_theme("nope", parsed)) {
      throw std::runtime_error("parse_theme failed");
    }
    for (const auto& [name, expected] : {
             std::pair{"default", pluss::table::display::DisplayTheme::Default},
             std::pair{"ember", pluss::table::display::DisplayTheme::Ember},
             std::pair{"obsidian", pluss::table::display::DisplayTheme::Obsidian},
             std::pair{"indigo", pluss::table::display::DisplayTheme::Indigo}}) {
      if (!pluss::table::display::parse_theme(name, parsed) || parsed != expected) {
        throw std::runtime_error(std::string("parse_theme failed for ") + name);
      }
    }

    std::cout << "DataFrame summary tab passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
