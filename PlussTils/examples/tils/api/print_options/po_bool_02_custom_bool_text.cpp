#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("Custom:", true, false,
              PrintOptions{.bool_style = BoolStyle::Custom, .bool_true = "OUI", .bool_false = "NON"});
  return 0;
}
