#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.logf(OutputLevel::Info, "disk {} used", 0.92, PrintOptions{.notation = Notation::Percent, .precision = 0});
  return 0;
}
