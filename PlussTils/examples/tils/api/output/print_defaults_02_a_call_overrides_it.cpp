#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1));
  out.writeln("engine baseline:", 3.14159);
  out.writeln("call override:", 3.14159, PrintOptions{}.fixed(4));
  return 0;
}
