// Removing and replacing each kind. A second ratio (spend over income) is added so the rule for several features is visible too.
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  const auto balance = Shelf(pluss::make_series({1200.0, 0.0, 450.0, 0.0, 0.0, 80.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, true, true}));
  const auto spend = Shelf(pluss::make_series({300.0, 0.0, 90.0, 120.0, 60.0, 40.0, -75.0}));
  const auto income = Shelf(pluss::make_series({2000.0, 0.0, 900.0, 800.0, 500.0, 0.0, 600.0}));
  DataFrame accounts({{"vs_balance", spend / balance}, {"vs_income", spend / income}});
  std::cout << "accounts\n" << accounts << "\n";

  // Each drop removes one kind of row and leaves the others where they are.
  std::cout << "dropnull\n" << accounts.dropnull() << "\n";
  std::cout << "dropnan\n" << accounts.dropnan() << "\n";
  std::cout << "dropinf\n" << accounts.dropinf() << "\n";
  std::cout << "dropinvalid\n" << accounts.dropinvalid() << "\n";

  // dropposinf / dropneginf remove one sign of infinity.
  std::cout << "dropposinf\n" << accounts.dropposinf() << "\n";

  // The default drops a row when ANY selected feature holds the kind; all = true only when EVERY selected feature does.
  std::cout << "dropnan({\"vs_balance\", \"vs_income\"}, true)\n" << accounts.dropnan({"vs_balance", "vs_income"}, true) << "\n";
}
