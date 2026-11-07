#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.writeln("captured line");
  out.to(std::cout);
  out.writeln("back on standard output");
  std::cout << "sink holds: " << sink.str();
  return 0;
}
