#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("rows:", std::vector<int>{1, 2, 3}, PrintOptions{.line_prefix = std::string("  | "), .multiline = true});
  return 0;
}
