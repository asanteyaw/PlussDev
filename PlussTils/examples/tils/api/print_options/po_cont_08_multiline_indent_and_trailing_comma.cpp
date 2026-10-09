#include <pluss/tils/aux/output.h>

#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("default indent:", std::vector<int>{1, 2, 3}, PrintOptions{.multiline = true});
  out.writeln("custom indent:", std::vector<int>{1, 2, 3}, PrintOptions{.multiline = true, .indent = "    "});
  out.writeln("trailing comma:", std::vector<int>{1, 2}, PrintOptions{.multiline = true, .trailing_comma = true});
  return 0;
}
