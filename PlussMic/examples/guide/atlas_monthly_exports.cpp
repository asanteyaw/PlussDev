// Combine monthly exports that share a schema: hold them in an Atlas, summarise each month, compare months, and catch a month whose schema drifted.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Shelf;

static DataFrame month(std::vector<std::string> products, std::vector<double> units, std::vector<double> revenue) {
  return DataFrame({{"product", pluss::make_note(products)}, {"units", pluss::make_series(units)}, {"revenue", pluss::make_series(revenue)}});
}

int main() {
  Atlas year({month({"widget", "gadget", "gizmo"}, {120, 80, 40}, {1200, 1600, 800}),
              month({"widget", "gadget", "gizmo"}, {150, 70, 55}, {1500, 1400, 1100}),
              month({"widget", "gadget", "gizmo"}, {90, 95, 60}, {900, 1900, 1200})},
             Shelf(pluss::make_note({"2026-09", "2026-10", "2026-11"})));
  std::cout << "months " << year.depth() << ", cases per month " << year.ncases() << ", features";
  for (const auto &name : year.column_names()) std::cout << " " << name;
  std::cout << "\n";

  // 1. One summary row per month: total units and revenue. aggregate_depth sees every month at once.
  auto totals = year.aggregate_depth([](const std::vector<DataFrame> &frames) {
    std::vector<double> units, revenue;
    for (const auto &f : frames) {
      units.push_back(f.features("units").as_series().sum().item<double>());
      revenue.push_back(f.features("revenue").as_series().sum().item<double>());
    }
    return DataFrame({{"units", pluss::make_series(units)}, {"revenue", pluss::make_series(revenue)}});
  });
  std::cout << totals.to_string() << "\n";

  // 2. The revenue per unit by month, and the best month.
  auto per_unit = totals.assign({{"revenue_per_unit", [](const DataFrame &f) { return f.features("revenue") / f.features("units"); }}});
  std::cout << per_unit.to_string() << "\n";

  // 3. One month's table, by label, and the latest month with head/tail.
  std::cout << "October:\n" << year.loc("2026-10").to_string() << "\n";
  std::cout << "latest month only: depth " << year.tail(1).depth() << "\n";

  // 4. A month that arrives with a renamed column breaks the shared schema; the Atlas refuses it.
  try {
    auto drifted = month({"widget", "gadget", "gizmo"}, {100, 100, 100}, {1000, 1000, 1000}).rename("units", "units_sold");
    Atlas bad({year.frame(0), drifted});
    std::cout << "unexpectedly accepted\n";
  } catch (const pluss::Error &e) {
    std::cout << "rejected: " << e.message() << "\n";
  }
}
