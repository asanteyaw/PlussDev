// Opens one PlussView window of a given kind and keeps it open, so a CI job can look at the screen.
//
//   pluss run view_probe.cpp table | table-filter | table-visit | catalog | catalog-tail
//
// The window is closed by whoever started the program (the job kills it after taking its screenshot).
#include <pluss/pluss.h>

#include <limits>
#include <string>

#include "kindle/catalog.h"
#include "table/display/Display.h"

int main(int argc, char** argv) {
  using namespace pluss::kindle::literals;
  namespace display = pluss::table::display;
  const std::string mode = argc > 1 ? argv[1] : "table";
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();

  if (mode.rfind("table", 0) == 0) {
    auto frame = pluss::table::DataFrame({
        {"balance", pluss::table::Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                                        pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}))},
        {"spend", pluss::table::Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}))},
        {"ratio", pluss::table::Shelf(pluss::make_series({0.25, nan, 0.2, inf, 0.0, 0.5, -inf}),
                                      pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}))},
    });
    display::DisplayOptions options;
    options.theme = display::DisplayTheme::Ember;
    if (mode == "table-filter") options.filter = display::DisplayFilter::Invalid;
    if (mode == "table-visit") {
      options.visit = display::DisplayVisit::Custom;
      options.visit_begin = 2;
      options.visit_end = 5;
    }
    display::show(frame, options);
    return 0;
  }

  auto catalog = pluss::kindle::Catalog({
      "accra_temp_c"_k = pluss::kindle::Stream(pluss::make_series({26.1, 27.2, 27.4, 27.9, 26.6, 27.3, 27.5})),
      "kumasi_temp_c"_k = pluss::kindle::Stream(pluss::make_series({25.0, nan, 26.2, 26.8})),
      "tamale_temp_c"_k = pluss::kindle::Stream(pluss::make_series({31.0, 30.4}),
                                                pluss::make_series(std::vector<bool>{true, false})),
  });
  catalog.show();
  return 0;
}
