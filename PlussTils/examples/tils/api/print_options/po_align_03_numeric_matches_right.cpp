#include <pluss/tils/aux/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  OutputEngine out;
  out.writelnf("right:   [{}]", -42, PrintOptions{.align = Align::Right, .width = 8});
  out.writelnf("numeric: [{}]", -42, PrintOptions{.align = Align::Numeric, .width = 8});
  return 0;
}
