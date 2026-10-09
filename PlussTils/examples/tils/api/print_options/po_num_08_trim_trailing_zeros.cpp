#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("not trimmed:", 1.2, PrintOptions{.notation = Notation::Fixed, .precision = 4});
  out.writeln("trimmed:", 1.2, PrintOptions{.notation = Notation::Fixed, .precision = 4, .trim_trailing_zeros = true});
  return 0;
}
