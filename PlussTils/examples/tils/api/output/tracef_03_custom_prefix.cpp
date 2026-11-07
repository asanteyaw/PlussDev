#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Trace, "[TRACE*]");
  out.tracef("formatted {} with a custom prefix", 5);
  return 0;
}
