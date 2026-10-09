#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("names:", std::vector<std::string>{"ama", "kofi, jr"}, PrintOptions{.quote_strings = true});
  return 0;
}
