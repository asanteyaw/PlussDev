// Opens DataFrame::show() in PlussView. Views: Table, Summary (count, missing, unique,
// mean/std/quartiles per numeric column), Correlation (tinted matrix), Missing and Outliers.
// Some columns have deliberately long names to show how headers wrap next to the sort arrows. Pick them from the View menu; Format (number format) is a
// cascading entry in the same menu.
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/mic.h"

// Usage: dataframe_summary_tab_demo [default|ember|nardo|brooklands|gulf|obsidian|indigo] [native] [big]
// "native" opens the same page in Apple's own web view instead of PlussView (it gets its own
// default palette unless a theme is also given). "big" uses 200,000 rows to show how long
// frames stay fast (the browser view then draws only the visible window of rows).
int main(int argc, char** argv) {
  using pluss::table::DataFrame;
  using pluss::table::Shelf;

  int n = 60;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "big") {
      n = 200000;
    }
  }
  const char* regions[] = {"North", "South", "East", "West"};
  std::vector<double> load, speed, temp;
  std::vector<std::int64_t> units;
  std::vector<std::string> region;
  std::vector<std::uint8_t> load_ok;
  for (int i = 0; i < n; ++i) {
    load.push_back(200.0 + 25.0 * std::sin(i / 6.0) + (i % 7));
    speed.push_back(45.0 + 6.0 * std::cos(i / 9.0) + (i % 5) * 0.3);
    temp.push_back(-3.0 + 0.4 * i + 2.0 * std::sin(i / 3.0));
    units.push_back(100 + (i * 37) % 90);
    region.push_back(regions[i % 4]);
    load_ok.push_back(i % 11 == 3 ? 0 : 1);  // a few missing load readings
  }

  load[7] = 420.0;   // outliers for the Outliers view
  temp[20] = 60.0;
  temp[33] = -40.0;
  std::vector<double> kwh, ltv, odd;
  for (int i = 0; i < n; ++i) {
    kwh.push_back(380.0 + 60.0 * std::sin(i / 7.0) + (i % 9) * 3.5);
    ltv.push_back(1200.0 + 35.0 * i + 90.0 * std::cos(i / 5.0));
    odd.push_back(static_cast<double>((i * 13) % 17) / 4.0);
  }
  const DataFrame df(
      {{"load_mw", Shelf(pluss::make_series(load), pluss::make_series(load_ok).to(pluss::codem::ScalarType::Bool))},
       {"speed_mph", Shelf(pluss::make_series(speed))},
       {"temp_c", Shelf(pluss::make_series(temp))},
       {"units", Shelf(pluss::make_series(units))},
       {"customer_lifetime_value_in_usd", Shelf(pluss::make_series(ltv))},
       {"average_monthly_electricity_consumption_kilowatt_hours", Shelf(pluss::make_series(kwh))},
       {"a_deliberately_extremely_long_column_name_that_runs_well_past_three_lines_to_show_truncation",
        Shelf(pluss::make_series(odd))},
       {"region", Shelf(pluss::make_note(region))}});
  pluss::table::display::DisplayOptions options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "big") {
      continue;
    } else if (arg == "native") {
      options.backend = pluss::table::display::DisplayBackend::NativeWebView;
    } else if (!pluss::table::display::parse_theme(arg, options.theme)) {
      return 2;  // unknown theme name
    }
  }
  df.show(options);
  return 0;
}
