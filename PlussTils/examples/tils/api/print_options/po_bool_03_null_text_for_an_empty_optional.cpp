#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("default:", std::optional<int>{});
  out.writeln("custom:", std::optional<int>{}, PrintOptions{.null_text = "<none>"});
  return 0;
}
