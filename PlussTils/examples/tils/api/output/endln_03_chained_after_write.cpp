#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.write("row:").write(" ", 7).endln().writeln("next row");
  return 0;
}
