// Fitting values into fixed-width places. A status board pads labels with a fill character, cuts a
// long service name with an ellipsis, and right-aligns a latency so its digits line up.
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  const std::string long_name = "inventory-sync-worker-eu-west-2";

  // Padding: a label is widened with dots in its own field, so the value after it starts in one column.
  out.writelnf("{:.<12}{}", "status", "ok");

  // Truncation: the long name is cut to 16 characters and ends with an ellipsis.
  out.writeln(long_name, PrintOptions{.truncate = 16, .ellipsis = "..."});

  // Alignment: each latency is right-aligned in eight characters, so the last digits line up.
  out.writeln(42, PrintOptions{.width = 8, .align = Align::Right});
  out.writeln(1180, PrintOptions{.width = 8, .align = Align::Right});

  // Rows built with the pattern form, where every field carries its own width.
  out.writelnf("{:<12}{:>8}", "checkout", 42);
  out.writelnf("{:<12}{:>8}", "auth", 7);
  return 0;
}
