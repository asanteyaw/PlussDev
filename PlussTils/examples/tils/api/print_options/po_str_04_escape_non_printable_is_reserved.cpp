#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  // escape_non_printable is accepted, but byte-level escaping is not implemented yet, so a tab
  // passes through unescaped. This is a documented, reserved field, not a claim of current behavior.
  out.writeln("with a tab:", "a\tb", PrintOptions{.escape_non_printable = true});
  out.writeln("without the option:", "a\tb");
  return 0;
}
