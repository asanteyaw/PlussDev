#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputOptions options;
  options.min_level = OutputLevel::Error;
  OutputEngine out{sink, options};
  out.logf(OutputLevel::Info, "dropped {}", 1);
  out.logf(OutputLevel::Error, "kept {}", 2);
  std::cout << sink.str();
  return 0;
}
