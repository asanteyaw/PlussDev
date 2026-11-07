#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputOptions options;
  options.min_level = OutputLevel::Info;
  options.auto_flush = true;
  OutputEngine out{sink, options};
  out.debug("below the minimum");
  out.info("at the minimum");
  std::cout << sink.str();
  return 0;
}
