// Prepare model inputs from a Catalog: standardise every numeric stream, leave text streams alone, check the result,
// and hand the prepared table to the DataFrame world.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  Catalog features({"household"_k = Stream(pluss::make_note({"h1", "h2", "h3", "h4", "h5", "h6"})),
                    "income"_k = Stream(pluss::make_series({52.0, 61.0, 48.0, 75.0, 43.0, 58.0})),
                    "size"_k = Stream(pluss::make_series({3.0, 4.0, 2.0, 5.0, 1.0, 3.0})),
                    "area"_k = Stream(pluss::make_series({85.0, 110.0, 62.0, 140.0, 45.0, 95.0}))});

  // 1. Standardise the numeric streams: each is replaced by (value - mean) / std. keyaxis().series() selects the numeric streams only.
  auto numeric = features.keyaxis().series();
  std::cout << "numeric streams: " << numeric.nkeys() << " of " << features.nkeys() << "\n";
  auto standardised = numeric.map([](const Stream &s) {
    // mean() and std() are one-record streams, so read them out as numbers before using them in arithmetic.
    const double mean = s.mean().as_series().items<double>()[0], sd = s.std().as_series().items<double>()[0];
    return (s - mean) / sd;
  });
  std::cout << standardised << "\n";

  // 2. Check: every standardised stream has mean 0 and standard deviation 1.
  for (const char *name : {"income", "size", "area"})
    std::cout << name << ": mean " << standardised.keys(name).mean().as_series().items<double>()[0] << ", std " << standardised.keys(name).std().as_series().items<double>()[0] << "\n";

  // 3. Put the identifier back next to the prepared streams and cross to the table model for the next stage.
  auto ready = standardised.set("household", features.keys("household"));
  auto table = ready.tabelize();
  std::cout << table.to_string() << "\n";
}
