#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("2500 ns:", 2500, PrintOptions{.humanize = Humanize::DurationNs});
  out.writeln("2.5 ms:", 2500000LL, PrintOptions{.humanize = Humanize::DurationNs});
  out.writeln("90 s:", 90000000000LL, PrintOptions{.humanize = Humanize::DurationNs});
  return 0;
}
