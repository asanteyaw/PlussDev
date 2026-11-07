#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Debug, "[DEBUG*]");
  out.debugf("formatted {} with a custom prefix", 5);
  return 0;
}
