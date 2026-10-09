#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("12345 rows:", 12345, PrintOptions{.humanize = Humanize::Metric});
  out.writeln("6.7 million:", 6700000LL, PrintOptions{.humanize = Humanize::Metric});
  return 0;
}
