#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Info, "[INFO*]");
  out.infof("formatted {} with a custom prefix", 5);
  return 0;
}
