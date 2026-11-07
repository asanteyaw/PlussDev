#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped(','));
  out.print_defaults(PrintOptions{});
  out.writeln("baseline cleared:", 1234567);
  std::cout << "stored baseline is empty: " << (out.print_defaults().grouping == '\0') << '\n';
  return 0;
}
