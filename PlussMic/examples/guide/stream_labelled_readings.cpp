// A stream whose records have names: attach station labels, look readings up by label, rank them and compare with a threshold.
#include <pluss/pluss.h>

#include <iostream>

using pluss::kindle::Stream;
using pluss::table::Shelf;

int main() {
  Stream wind(pluss::make_series({12.4, 31.0, 8.2, 25.7, 40.3, 19.9}));
  auto named = wind.with_label(Shelf(pluss::make_note({"harbour", "ridge", "valley", "airport", "summit", "coast"})));
  std::cout << "labelled: " << (named.has_label() ? "yes" : "no") << ", records " << named.nrecords() << "\n";
  std::cout << named << "\n";

  // 1. Look records up by position through the labels: the label stream says which station each position is.
  auto labels = named.label()->as_note().host();
  const auto &names = labels.vector();
  auto values = named.as_series().items<double>();

  // 2. Rank the stations, strongest wind first, and report the top three by name.
  auto order = named.as_series().argsort(0, true).items<std::int64_t>();
  std::cout << "strongest winds:";
  for (int i = 0; i < 3; ++i) std::cout << " " << names[static_cast<std::size_t>(order[static_cast<std::size_t>(i)])] << " (" << values[static_cast<std::size_t>(order[static_cast<std::size_t>(i)])] << ")";
  std::cout << "\n";

  // 3. A threshold alert: which stations exceed 25 km/h? filter keeps the labels with the values.
  auto windy = named.filter(named.as_series().gt(25.0));
  auto windy_names = windy.label()->as_note().host();
  std::cout << "above 25:";
  for (const auto &n : windy_names.vector()) std::cout << " " << n;
  std::cout << " (" << windy.nrecords() << " of " << named.nrecords() << ")\n";

  // 4. Clearing the labels returns the plain stream.
  std::cout << "after clear_label: " << (named.clear_label().has_label() ? "yes" : "no") << "\n";
}
