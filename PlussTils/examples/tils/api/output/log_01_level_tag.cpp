#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.log(OutputLevel::Info, "service started on port", 8080);
  out.log(OutputLevel::Error, "could not bind");
  return 0;
}
