#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("value", PrintOptions{.prefix = std::string(">> "), .suffix = std::string(" <<")});
  return 0;
}
