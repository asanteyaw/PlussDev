#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out;
  OutputEngine& same = out.to(sink);
  std::cout << std::boolalpha << "returns the same engine: " << (&same == &out) << '\n';
  same.writeln("chained");
  std::cout << "sink holds: " << sink.str();
  return 0;
}
