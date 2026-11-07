#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputOptions options;
  options.min_level = OutputLevel::Warn;
  OutputEngine out{sink, options};
  out.log(OutputLevel::Debug, "dropped");
  out.log(OutputLevel::Warn, "kept");
  std::cout << sink.str();
  return 0;
}
