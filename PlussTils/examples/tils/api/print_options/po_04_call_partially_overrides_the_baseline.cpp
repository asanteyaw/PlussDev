#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped('_').fixed(1));
  // The call sets only the precision; the grouping still comes from the baseline.
  out.writeln("partial override:", 1234567.25, PrintOptions{}.fixed(0));
  return 0;
}
