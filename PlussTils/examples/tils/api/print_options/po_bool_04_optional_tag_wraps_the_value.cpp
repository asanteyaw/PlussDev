#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("untagged:", std::optional<int>{7}, std::optional<int>{});
  out.writeln("tagged:", std::optional<int>{7}, std::optional<int>{}, PrintOptions{.optional_tag = true});
  return 0;
}
