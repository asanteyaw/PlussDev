// Replacing each kind with a value of its own: missing readings, invalid computations and unbounded ones are not the same thing.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto ratio = spend / balance;
  std::cout << "ratio\n" << ratio << "\n";

  // Each filler touches only its own kind; the others are left exactly as they were.
  std::cout << "fillnull(-9)\n" << ratio.fillnull(-9.0) << "\n";
  std::cout << "fillnan(0)\n" << ratio.fillnan(0.0) << "\n";
  std::cout << "fillinf(99)\n" << ratio.fillinf(99.0) << "\n";

  // A cap for +inf and a floor for -inf: one call each.
  std::cout << "fillposinf(10).fillneginf(-10)\n" << ratio.fillposinf(10.0).fillneginf(-10.0) << "\n";

  // A different value for every kind: chain the fillers.
  std::cout << "one value per kind\n" << ratio.fillnull(-9.0).fillnan(0.0).fillposinf(10.0).fillneginf(-10.0) << "\n";

  // The all-in-one: the SAME value for every kind. Use it when the kinds need no distinction.
  std::cout << "fillinvalid(0)\n" << ratio.fillinvalid(0.0) << "\n";
}
