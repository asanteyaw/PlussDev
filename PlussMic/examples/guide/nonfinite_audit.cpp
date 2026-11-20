// Auditing a computed column: how many cells are missing, NaN, +inf, -inf and ordinary, and what each answer tells you to do.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto income = Shelf(pluss::make_series({2000.0, 0.0, 900.0, 800.0, 500.0, 0.0, 600.0}));
  DataFrame accounts({{"balance", balance}, {"spend", spend}, {"vs_balance", spend / balance}, {"vs_income", spend / income}});

  // One call, one row per feature. The five counts never overlap and always add up to the total.
  std::cout << accounts.count_kinds() << "\n";

  // Read one kind across every feature: which of them contain NaN, and how many cells?
  std::cout << "NaN per feature\n" << accounts.count_kinds().features("nan") << "\n";

  // The three one-cell questions behind the report are available on their own.
  std::cout << "missing in balance: " << accounts.features("balance").nnull() << "\n";
  std::cout << "NaN in vs_balance:  " << accounts.features("vs_balance").nnan() << "\n";
  std::cout << "+inf in vs_income:  " << accounts.features("vs_income").nposinf() << "\n";
  std::cout << "-inf in vs_balance: " << accounts.features("vs_balance").nneginf() << "\n";
}
