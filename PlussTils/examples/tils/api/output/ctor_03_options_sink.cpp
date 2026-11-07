#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  OutputOptions options;
  options.min_level = OutputLevel::Warn;  // info is dropped
  OutputEngine out{captured, options};
  out.info("dropped");
  out.warn("kept");
  std::cout << "captured: " << captured.str();
  return 0;
}
