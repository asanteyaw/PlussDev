// Format strings in a configuration file can drift from the values they are given. Strict mode
// turns a mismatch into an error, and a missing argument can be shown as a visible marker instead.
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <stdexcept>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};

  // A pattern with two placeholders is given three values. Strict mode refuses the extra one.
  try {
    out.writelnf("order {} shipped to {}", 1001, "Lagos", "express", PrintOptions{.strict_args = true});
  } catch (const std::runtime_error& error) {
    out.writeln("strict mode refused the line:", error.what());
  }

  // A pattern with three placeholders is given two values. The missing one shows the marker.
  out.writelnf("order {} shipped to {} on {}", 1001, "Lagos", PrintOptions{.missing_arg = "<?>"});
  return 0;
}
