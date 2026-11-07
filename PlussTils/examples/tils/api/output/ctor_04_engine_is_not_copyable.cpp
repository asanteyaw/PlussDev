#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  std::cout << "copy constructible: " << std::is_copy_constructible_v<OutputEngine> << '\n';
  std::cout << "copy assignable: " << std::is_copy_assignable_v<OutputEngine> << '\n';
  return 0;
}
