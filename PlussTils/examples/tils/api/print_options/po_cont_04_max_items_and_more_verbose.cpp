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
  const std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
  out.writeln("capped:", v, PrintOptions{.max_items = 3});
  out.writeln("capped with a count:", v, PrintOptions{.max_items = 3, .more_verbose = true});
  return 0;
}
