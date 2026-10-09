#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("total:", 42, PrintOptions{}.with_end(" (estimated)"));
  return 0;
}
