#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Warn, "[!!]");
  out.warn("custom warning tag");
  out.info("info keeps its tag");
  return 0;
}
