// The sales report again, the stream-first way: a Catalog of named streams, a derived stream, a filter, an ordering,
// and one summary per region from grouping.
#include <pluss/pluss.h>

#include <iostream>
#include <map>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog sales({"city"_k = Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale", "Takoradi", "Cape Coast"})),
                 "region"_k = Stream(pluss::make_note({"south", "south", "middle", "north", "south", "middle"})),
                 "units"_k = Stream(pluss::make_series({10.0, 7.0, 12.0, 5.0, 9.0, 6.0})),
                 "price"_k = Stream(pluss::make_series({3.0, 4.0, 2.5, 5.0, 3.5, 4.5}))});
  std::cout << sales << "\n";

  // 1. A derived stream: set adds a new named stream computed from existing ones.
  auto enriched = sales.set("revenue", sales.keys("units") * sales.keys("price"));
  std::cout << "streams: " << enriched.nkeys() << ", rows: " << enriched.nrows() << "\n";

  // 2. Filter by a condition on a stream, then order by revenue.
  auto strong = enriched.where(enriched.keys("revenue").as_series().ge(28.0));
  std::cout << "rows with revenue of at least 28: " << strong.nrows() << "\n";
  std::cout << enriched.sort("revenue", false).head(3) << "\n";

  // 3. Grouping splits a catalog into one catalog per key value; summarise each.
  auto groups = enriched.group().by("region");
  std::map<std::string, Catalog> ordered(groups.begin(), groups.end());
  for (const auto &[region, part] : ordered) {
    std::cout << region << ": " << part.nrows() << " cities, units " << part.keys("units").sum().as_series().items<double>()[0] << ", mean revenue "
              << part.keys("revenue").mean().as_series().items<double>()[0] << "\n";
  }
}
