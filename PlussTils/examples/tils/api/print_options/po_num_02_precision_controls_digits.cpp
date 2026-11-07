#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("default fixed precision:", 1.0 / 3.0, PrintOptions{.notation = Notation::Fixed});
  out.writeln("precision 2:", 1.0 / 3.0, PrintOptions{.notation = Notation::Fixed, .precision = 2});
  return 0;
}
