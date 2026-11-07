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
  out.writeln("names:", std::vector<std::string>{"zero", "one", "two"}, PrintOptions{.show_indices = true});
  return 0;
}
