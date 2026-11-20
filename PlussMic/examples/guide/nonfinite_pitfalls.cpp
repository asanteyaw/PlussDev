// Four mistakes that come from treating the kinds as one, each with what actually happens and the call that was meant.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto ratio = spend / balance;
  std::cout << "ratio\n" << ratio << "\n";

  // 1. Testing for NaN with ==. A NaN is not equal to anything, itself included, so the comparison never finds it. A missing cell stays missing.
  std::cout << "ratio == NaN  (finds nothing)\n" << (ratio == nan) << "\n";
  std::cout << "ratio.isnan()  (finds it)\n" << ratio.isnan() << "\n";

  // 2. dropnull() to remove "the bad rows". It removes the missing cell only; the NaN and the infinities are values and stay.
  std::cout << "dropnull() keeps " << ratio.dropnull().size() << " of " << ratio.size() << " cells; NaN still present: " << ratio.dropnull().nnan()
            << "\n";

  // 3. fillnull(0) to repair the column. It fills the missing cell and leaves the NaN and both infinities exactly as they were.
  const auto repaired = ratio.fillnull(0.0);
  std::cout << "after fillnull(0): missing " << repaired.nnull() << ", NaN " << repaired.nnan() << ", infinite " << repaired.nposinf() + repaired.nneginf()
            << "\n\n";

  // 4. A mean over a column that holds a NaN. The missing cell is skipped, the NaN is not, so one invalid division makes the whole mean NaN.
  const auto finite_only = ratio.dropnan().dropinf();
  std::cout << "mean(ratio)                      " << ratio.mean().to_dbl() << "\n";
  std::cout << "mean(ratio.dropinvalid())        " << ratio.dropinvalid().mean().to_dbl() << "\n";
  std::cout << "mean(ratio.dropnan().dropinf())  " << finite_only.mean().to_dbl() << "\n\n";

  // Comparisons follow the kinds too: inf > 0.4 is true, NaN > 0.4 is false, and a missing cell gives a missing answer (a query drops it).
  std::cout << "ratio > 0.4\n" << (ratio > 0.4) << "\n";
  DataFrame accounts({{"ratio", ratio}});
  std::cout << accounts.query("ratio > 0.4") << "\n";
}
