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
  const std::map<std::string, int> m{{"c", 3}, {"a", 1}, {"b", 2}};
  out.writeln("map is already ordered by its own key type, so this is about larger keys:", m, PrintOptions{.sort_keys = true});
  return 0;
}
