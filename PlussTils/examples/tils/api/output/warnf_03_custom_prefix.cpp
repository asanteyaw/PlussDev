#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Warn, "[WARN*]");
  out.warnf("formatted {} with a custom prefix", 5);
  return 0;
}
