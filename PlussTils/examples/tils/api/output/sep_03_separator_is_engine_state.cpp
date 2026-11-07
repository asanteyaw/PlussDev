#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.sep(", ");
  out.writeln(1, 2, 3);
  out.writeln(4, 5);
  std::cout << sink.str();
  return 0;
}
