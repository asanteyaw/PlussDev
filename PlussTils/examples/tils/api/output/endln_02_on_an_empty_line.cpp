#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.endln();
  std::cout << "bytes written by endln alone: " << sink.str().size() << '\n';
  return 0;
}
