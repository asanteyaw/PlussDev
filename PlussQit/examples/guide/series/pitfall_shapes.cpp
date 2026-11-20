// Broadcasting aligns shapes from the right. A column and a row make a table; two incompatible axes are an error.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto column = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);   // shape [3, 1]
  auto row = pluss::make_series({10.0, 20.0, 30.0, 40.0});          // shape [4]
  show("column + row gives a 3 x 4 table", column + row);
  auto wrong = pluss::make_series({1.0, 2.0, 3.0});                 // shape [3]
  try {
    show("row + wrong", row + wrong);
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
  // Forgetting unsqueeze: a [3] plus a [4] cannot align; the [3, 1] column is what makes it a table.
}
