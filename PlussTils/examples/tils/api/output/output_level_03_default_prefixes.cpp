#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  const OutputOptions options;
  for (const auto& prefix : options.level_prefixes) std::cout << prefix << '\n';
  return 0;
}
