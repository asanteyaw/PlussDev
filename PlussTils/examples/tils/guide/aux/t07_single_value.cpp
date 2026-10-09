// transmit also works on one value that is not a range. It calls the function on the value and
// returns the result, so the same call reads the same whether the input is one value or many.
// A string is a range of characters, so it takes the range form instead; see the pitfalls.
#include <pluss/tils/aux/transmit.h>

#include <iostream>

struct Invoice {
  int items;
  double subtotal;
};

int main() {
  const Invoice invoice{3, 120.0};
  const double with_tax = pluss::tils::transmit(
      [](const Invoice& i) { return i.subtotal * 1.2; }, invoice);
  std::cout << "subtotal with tax: " << with_tax << '\n';

  const int doubled = pluss::tils::transmit([](int x) { return x * 2; }, 21);
  std::cout << "doubled: " << doubled << '\n';
  return 0;
}
