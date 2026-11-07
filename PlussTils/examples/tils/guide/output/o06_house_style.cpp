// A house style for a finance team: every number groups thousands and shows two decimals. The
// style is set once on the engine. One call overrides it, and the rest keep it.
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.print_defaults(PrintOptions{}.grouped(',').fixed(2));

  out.writeln("invoice total:", 1234567.5);                       // house style applies
  out.writeln("tax, zero decimals:", 1234567.5, PrintOptions{}.fixed(0));  // this call overrides
  out.writeln("discount:", 250.0);                                // house style again

  out.print_defaults(PrintOptions{});                             // clear the house style
  out.writeln("plain again:", 1234567.5);
  return 0;
}
