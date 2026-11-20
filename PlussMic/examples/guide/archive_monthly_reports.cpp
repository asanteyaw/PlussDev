// Monthly catalogs that do not have the same number of rows (products come and go): hold them in an Archive, total each month,
// and compare the months.
#include <pluss/pluss.h>

#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace pluss::kindle::literals;
using pluss::kindle::Archive;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

static Catalog month(std::vector<std::string> product, std::vector<double> units, std::vector<double> revenue) {
  return Catalog({"product"_k = Stream(pluss::make_note(product)), "units"_k = Stream(pluss::make_series(units)), "revenue"_k = Stream(pluss::make_series(revenue))});
}

int main() {
  Archive year({month({"widget", "gadget", "gizmo"}, {120, 80, 40}, {1200, 1600, 800}),
                month({"widget", "gadget", "gizmo", "gear"}, {150, 70, 55, 30}, {1500, 1400, 1100, 900}),
                month({"widget", "gizmo"}, {90, 60}, {900, 1200})},
               Stream(pluss::make_note({"2026-09", "2026-10", "2026-11"})));
  std::cout << "months " << year.depth() << ", products per month from " << year.minrecords() << " to " << year.maxrecords() << "\n";

  // 1. One line per month, computed across the whole archive.
  auto totals = year.aggregate_depth([](const std::vector<Catalog> &catalogs) {
    std::vector<double> units, revenue;
    for (const auto &c : catalogs) {
      units.push_back(c.keys("units").sum().as_series().items<double>()[0]);
      revenue.push_back(c.keys("revenue").sum().as_series().items<double>()[0]);
    }
    return Catalog({"units"_k = Stream(pluss::make_series(units)), "revenue"_k = Stream(pluss::make_series(revenue))});
  });
  std::cout << totals << "\n";

  // 2. Which product is present in every month? Intersect the product names, month by month.
  std::set<std::string> everywhere;
  for (std::int64_t i = 0; i < year.depth(); ++i) {
    const auto names = year.dloc(i).keys("product").as_note().host();
    std::set<std::string> now(names.vector().begin(), names.vector().end());
    if (i == 0) everywhere = now;
    else {
      std::set<std::string> keep;
      for (const auto &n : everywhere) if (now.count(n)) keep.insert(n);
      everywhere = keep;
    }
  }
  std::cout << "products sold in every month:";
  for (const auto &n : everywhere) std::cout << " " << n;
  std::cout << "\n";

  // 3. November alone, by label.
  std::cout << year.loc("2026-11") << "\n";
}
