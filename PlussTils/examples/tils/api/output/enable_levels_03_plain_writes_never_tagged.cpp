#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("plain write has no level");
  out.enable_levels(true);
  out.writeln("still no level, because writeln is not a log call");
  return 0;
}
