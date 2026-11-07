#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.prefix(OutputLevel::Error, "[FAIL]");
  out.error("failed");
  out.debug("debug uses the default");
  std::cout << sink.str();
  return 0;
}
