#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  const OutputOptions options;
  std::cout << "separator: [" << options.separator << "]\n";
  std::cout << "levels enabled: " << options.levels_enabled << '\n';
  std::cout << "auto flush: " << options.auto_flush << '\n';
  std::cout << "info prefix: " << options.level_prefixes[2] << '\n';
  return 0;
}
