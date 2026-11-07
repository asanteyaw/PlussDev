#include <pluss/tils/kit/output.h>

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
  const std::vector<std::vector<int>> nested{{1, 2}, {3, 4}};
  out.writeln("full depth:", nested);
  out.writeln("depth 1:", nested, PrintOptions{.max_depth = 1});
  return 0;
}
