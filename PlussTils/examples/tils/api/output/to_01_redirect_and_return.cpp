#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream first, second;
  OutputEngine out{first};
  out.writeln("to the first sink");
  out.to(second);
  out.writeln("to the second sink");
  std::cout << "first: " << first.str();
  std::cout << "second: " << second.str();
  return 0;
}
