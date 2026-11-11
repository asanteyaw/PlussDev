#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/mic.h"
#include "table/Atlas.h"
#include "table/display/Display.h"

namespace {

void require(const std::string& html, std::string_view fragment) {
  if (html.find(fragment) == std::string::npos) {
    throw std::runtime_error("Atlas view is missing " + std::string(fragment));
  }
}

} // namespace

int main() {
  try {
    using pluss::table::Atlas;
    using pluss::table::DataFrame;
    using pluss::table::Slate;

    const Atlas atlas(
        {
            DataFrame({{"city", Slate(pluss::make_note({"Edmonton"}))},
                       {"count", Slate(pluss::make_series(std::vector<std::int64_t>{12}))}}),
            DataFrame({{"city", Slate(pluss::make_note({"Banff"}))},
                       {"count", Slate(pluss::make_series(std::vector<std::int64_t>{3}))}}),
            DataFrame({{"city", Slate(pluss::make_note({"Jasper"}))},
                       {"count", Slate(pluss::make_series(std::vector<std::int64_t>{5}))}}),
        },
        Slate(pluss::make_note({"Baseline", "Current & plan", "Forecast"})));

    const std::string html = pluss::table::display::render_html(atlas);
    for (const auto control : {"atlas-first", "atlas-previous", "atlas-next", "atlas-last",
                               "atlas-show-all", "atlas-tab-0", "atlas-tab-1", "atlas-tab-2"}) {
      require(html, std::string("id=\"") + control + "\"");
    }
    require(html, "Baseline (1 of 3)");
    require(html, "Current &amp; plan");
    require(html, "role=\"status\"");
    require(html, "class=\"atlas-panel active\" data-frame-index=\"0\"");
    require(html, "class=\"atlas-panel\" data-frame-index=\"1\"");
    require(html, "class=\"atlas-panel\" data-frame-index=\"2\"");
    std::cout << "Atlas view controls passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
