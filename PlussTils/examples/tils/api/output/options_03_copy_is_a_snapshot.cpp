#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  OutputOptions snapshot = out.options();
  snapshot.separator = "::";
  out.writeln("a", "b");
  std::cout << "engine separator after editing the copy: [" << out.options().separator << "]\n";
  return 0;
}
