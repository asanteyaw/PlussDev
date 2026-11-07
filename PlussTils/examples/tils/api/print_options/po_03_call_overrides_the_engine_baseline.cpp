#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped('_').fixed(1));
  out.writeln("baseline only:", 1234567.25);
  out.writeln("call overrides both fields:", 1234567.25, PrintOptions{}.grouped(',').fixed(3));
  return 0;
}
