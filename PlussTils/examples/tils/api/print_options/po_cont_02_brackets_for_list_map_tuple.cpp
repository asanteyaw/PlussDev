#include <pluss/tils/auxiliary/output.h>

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
  out.writeln("list:", std::vector<int>{1, 2, 3}, PrintOptions{.list_brackets = std::array<std::string, 2>{"<< ", " >>"}});
  out.writeln("map:", std::map<std::string, int>{{"a", 1}}, PrintOptions{.map_brackets = std::array<std::string, 2>{"{ ", " }"}});
  out.writeln("tuple:", std::tuple{1, 2, 3}, PrintOptions{.tuple_brackets = std::array<std::string, 2>{"[|", "|]"}});
  return 0;
}
