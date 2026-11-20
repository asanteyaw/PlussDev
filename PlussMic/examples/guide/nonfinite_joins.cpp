// Join keys: a missing key matches nothing, not even another missing key; a NaN key is a value, so it matches another NaN key.
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  // Left keys: 1, NaN, 3 and a missing key (the 0.0 under the mask is only a placeholder).
  Catalog left({"k"_k = Stream(pluss::make_series({1.0, nan, 3.0, 0.0}), pluss::make_series(std::vector<bool>{true, true, true, false})),
                "x"_k = Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0}))});
  // Right keys: NaN, 3 and a missing key.
  Catalog right({"k"_k = Stream(pluss::make_series({nan, 3.0, 0.0}), pluss::make_series(std::vector<bool>{true, true, false})),
                 "y"_k = Stream(pluss::make_series({1.0, 2.0, 3.0}))});

  pluss::kindle::CatalogJoinSpec spec;
  spec.how = pluss::relational::JoinHow::Inner;
  spec.left_keys = {"k"};
  spec.right_keys = {"k"};
  auto joined = left.join(right, spec);
  std::cout << "x\n" << joined.keys("x") << "\n" << "y\n" << joined.keys("y") << "\n";
}
