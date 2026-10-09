#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("default point:", 3.5, PrintOptions{.notation = Notation::Fixed, .precision = 2});
  out.writeln("comma point:", 3.5, PrintOptions{.notation = Notation::Fixed, .precision = 2, .decimal_point = ','});
  return 0;
}
