#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  OutputOptions options;
  options.auto_flush = true;
  out.set_options(options).writeln("chained after set_options");
  return 0;
}
