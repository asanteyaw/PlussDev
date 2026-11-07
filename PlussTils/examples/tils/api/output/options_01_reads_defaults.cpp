#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  OutputEngine out;
  const OutputOptions now = out.options();
  std::cout << "separator: [" << now.separator << "]\n";
  std::cout << "levels enabled: " << now.levels_enabled << '\n';
  std::cout << "min level is trace: " << (now.min_level == OutputLevel::Trace) << '\n';
  return 0;
}
