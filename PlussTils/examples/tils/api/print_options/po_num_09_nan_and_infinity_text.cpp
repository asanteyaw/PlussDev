#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln(std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::infinity(),
              -std::numeric_limits<double>::infinity(),
              PrintOptions{.nan_text = "missing", .pos_inf_text = "overflow", .neg_inf_text = "underflow"});
  return 0;
}
