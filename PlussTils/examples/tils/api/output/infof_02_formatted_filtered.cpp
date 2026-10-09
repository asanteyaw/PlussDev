#include <pluss/tils/aux/output.h>

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
  out.infof("value {}", 9);
  std::cout << "captured bytes: " << sink.str().size() << '\n';
  return 0;
}
