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
  out.writeln("items:", std::vector<int>{1, 2, 3}, PrintOptions{.item_sep = std::string(" ; ")});
  out.writeln("map:", std::map<std::string, int>{{"a", 1}}, PrintOptions{.kv_sep = std::string(" -> ")});
  return 0;
}
