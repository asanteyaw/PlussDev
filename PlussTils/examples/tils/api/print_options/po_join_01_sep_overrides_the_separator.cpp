#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("a", "b", "c", PrintOptions{.sep = std::string(" / ")});
  out.writeln("a", "b", "c");
  return 0;
}
