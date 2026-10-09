#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  OutputEngine out;
  out.sep(" ; ");
  out.enable_levels(false);
  std::cout << "separator now: [" << out.options().separator << "]\n";
  std::cout << "levels now enabled: " << out.options().levels_enabled << '\n';
  return 0;
}
