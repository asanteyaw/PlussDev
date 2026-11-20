// item<T>() is for a single value. Anything else needs host<T>() or a reduction first.
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << "mean as a number: " << x.mean().item<double>() << "\n";
  try {
    std::cout << x.item<double>() << "\n";
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
  std::vector<double> values = x.items<double>();
  std::cout << "all three values through items<double>: " << values[0] << " " << values[1] << " " << values[2] << "\n";
  std::cout << "one element through select: " << x.select(0, 1).item<double>() << "\n";
}
