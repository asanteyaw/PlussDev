#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.writeln("flushed by option", PrintOptions{.flush = true});
  std::cout << "sink holds: " << sink.str();
  return 0;
}
