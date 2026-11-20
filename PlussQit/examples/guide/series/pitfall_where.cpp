// where: keep a value where a condition holds and take a replacement elsewhere, and the one mistake that actually happens, a condition that
// does not line up with the data.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto big = x.gt(2.5);
  // x.where(big, 0) keeps x where `big` is true and takes 0 elsewhere; the free form writes the condition first.
  show("x.where(big, 0)              ", x.where(big, 0.0));
  show("pluss::series::where(big, x, 0)", pluss::series::where(big, x, 0.0));

  // A condition with five flags for four values cannot line up with the data.
  try {
    auto flags = pluss::make_series({true, false, true, false, true});
    show("never printed", x.where(flags, 0.0));
  } catch (const pluss::Error &e) {
    std::cout << "wrong length: " << e.message() << "\n";
  }
}
