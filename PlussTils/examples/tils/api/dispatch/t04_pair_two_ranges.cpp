// Pairing two lists element by element. A price list and a quantity list are combined into line
// totals. When the lists differ in length, the result stops at the shorter one.
#include <pluss/tils/aux/transmit.h>

#include <iostream>
#include <vector>

int main() {
  const std::vector<double> prices{2.50, 10.00, 4.25};
  const std::vector<int> quantities{4, 1, 3, 9};

  const auto totals = pluss::tils::transmit(
      [](double price, int quantity) { return price * quantity; }, prices, quantities);

  for (double total : totals) std::cout << "line total: " << total << '\n';
  std::cout << "lines: " << totals.size() << " (the ninth quantity has no price)\n";
  return 0;
}
