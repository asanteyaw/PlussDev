#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped('_'));
  out.writeln("baseline grouping:", 1000000);
  out.writeln("again:", 2500000);
  return 0;
}
