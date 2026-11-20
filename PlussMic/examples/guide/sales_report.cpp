// A sales report from a table: derive revenue, filter, rank, and summarise by region, then by overall totals.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;

int main() {
  auto sales = DataFrame({
      {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale", "Takoradi", "Cape Coast"})},
      {"region", pluss::make_note({"south", "south", "middle", "north", "south", "middle"})},
      {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0, 9.0, 6.0})},
      {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0, 3.5, 4.5})},
  }).title("sales");
  std::cout << sales.to_string() << "\n";

  // 1. A derived feature: revenue = units x price. assign takes a function of the frame, so it can use any existing feature.
  auto enriched = sales.assign({{"revenue", [](const DataFrame &f) { return f.features("units") * f.features("price"); }}});
  std::cout << enriched.to_string() << "\n";

  // 2. Filter with a text predicate, then order the survivors by revenue, highest first.
  auto strong = enriched.query("revenue >= 28");
  std::cout << "cities with revenue of at least 28: " << strong.ncases() << "\n";
  std::cout << enriched.arrange({"revenue"}, false).head(3).to_string() << "\n";

  // 3. One row per region: total units, mean revenue and the number of cities.
  auto by_region = enriched.group({.dropnull = true, .sort = true})
                       .by({"region"})
                       .agg({{"cities", "city", DataFrame::GroupOp::Count},
                             {"units_sum", "units", DataFrame::GroupOp::Sum},
                             {"revenue_mean", "revenue", DataFrame::GroupOp::Mean}});
  std::cout << by_region.to_string() << "\n";

  // 4. The whole table in one row.
  auto total = enriched.aggregate({{"total_units", "units", DataFrame::SummaryOp::Sum}, {"total_revenue", "revenue", DataFrame::SummaryOp::Sum}}, false);
  std::cout << total.to_string() << "\n";
}
