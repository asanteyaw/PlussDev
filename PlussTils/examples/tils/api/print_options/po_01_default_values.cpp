#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  const PrintOptions p;
  std::cout << "sep is unset: " << !p.sep.has_value() << '\n';
  std::cout << "width: " << p.width << '\n';
  std::cout << "align is Inherit: " << (p.align == Align::Inherit) << '\n';
  std::cout << "grouping is none: " << (p.grouping == '\0') << '\n';
  return 0;
}
