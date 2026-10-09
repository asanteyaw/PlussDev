#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.min_level = OutputLevel::Error;
  OutputEngine out;
  out.set_options(options);
  out.info("filtered out");
  out.error("kept");
  return 0;
}
