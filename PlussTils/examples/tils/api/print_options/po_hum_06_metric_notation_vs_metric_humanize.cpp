#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  // Both reach the same scaling helper, so Notation::Metric and Humanize::Metric agree.
  out.writeln("via notation:", 12345.0, PrintOptions{.notation = Notation::Metric});
  out.writeln("via humanize:", 12345, PrintOptions{.humanize = Humanize::Metric});
  out.writeln("small values are not scaled down:", 0.0025, PrintOptions{.notation = Notation::Metric});
  return 0;
}
