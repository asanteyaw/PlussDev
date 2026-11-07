#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("names:", std::vector<std::string>{"ama", "kofi"},
              PrintOptions{.quote_strings = true, .quote_char = '\''});
  return 0;
}
