#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("hex lower:", 3735928559U, PrintOptions{.notation = Notation::Hex, .alt = true});
  out.writeln("hex upper:", 3735928559U, PrintOptions{.notation = Notation::Hex, .alt = true, .uppercase = true});
  out.writeln("scientific upper:", 12345.678, PrintOptions{.notation = Notation::Scientific, .precision = 2, .uppercase = true});
  return 0;
}
