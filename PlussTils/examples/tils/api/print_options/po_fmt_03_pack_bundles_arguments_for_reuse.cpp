#include <pluss/tils/aux/output.h>

#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  auto bundle = pack(std::string("ama"), 7);
  out.writelnf("{} is {}", bundle);
  out.writelnf("{1} then {0}", bundle);
  return 0;
}
