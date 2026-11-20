// Four kinds of cell in one column: a spend-to-balance ratio over seven accounts. One account has no balance on record (NULL); three have a zero
// balance, so their ratios are 0 / 0 (NaN), 120 / 0 (+inf) and -75 / 0 (-inf); and the other three are ordinary.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  // The balance of the fifth account was never recorded: the validity mask marks it missing (the 0.0 under it is only a placeholder).
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));

  const auto ratio = spend / balance;
  DataFrame accounts({{"balance", balance}, {"spend", spend}, {"ratio", ratio}});
  std::cout << accounts << "\n";
}
