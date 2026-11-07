#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  OutputEngine out{captured};  // writes into the string stream
  out.writeln("held in the capture");
  std::cout << "captured text: " << captured.str();
  return 0;
}
