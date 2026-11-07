#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.separator = " / ";
  OutputEngine out;
  out.set_options(options);
  out.writeln("x", "y", "z");
  return 0;
}
