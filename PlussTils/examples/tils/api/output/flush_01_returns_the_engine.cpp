#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.write("pending").flush().write(" then more").endln();
  std::cout << sink.str();
  return 0;
}
