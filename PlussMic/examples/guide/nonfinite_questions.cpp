// The question you ask decides the answer. One column holding every kind of cell, and every predicate side by side.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto ratio = spend / balance;

  // Every column below is the answer to one question about the `ratio` column.
  DataFrame questions({
      {"ratio", ratio},
      {"isnull", ratio.isnull()},
      {"isnan", ratio.isnan()},
      {"isinf", ratio.isinf()},
      {"isposinf", ratio.isposinf()},
      {"isneginf", ratio.isneginf()},
      {"invalid", ratio.invalid()},
  });
  std::cout << questions << "\n";

  // The complements: notnan is true for NULL (a missing cell is not a NaN) and notinf is true for NaN.
  DataFrame complements({{"ratio", ratio}, {"notnull", ratio.notnull()}, {"notnan", ratio.notnan()}, {"notinf", ratio.notinf()}});
  std::cout << complements << "\n";
}
