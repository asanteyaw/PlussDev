#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.writeln("flushed now", PrintOptions{.flush = true});
  std::cout << "sink holds: " << sink.str();
  return 0;
}
