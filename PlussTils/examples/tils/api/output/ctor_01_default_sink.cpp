#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;  // writes to std::cout
  out.writeln("default engine writes", "to standard output");
  return 0;
}
