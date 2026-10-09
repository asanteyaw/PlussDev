#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.logf(OutputLevel::Warn, "retry {} of {} after {} ms", 2, 5, 400);
  return 0;
}
