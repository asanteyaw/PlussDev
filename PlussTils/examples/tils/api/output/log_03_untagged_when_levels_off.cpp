#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.enable_levels(false);
  out.log(OutputLevel::Info, "level text removed");
  return 0;
}
