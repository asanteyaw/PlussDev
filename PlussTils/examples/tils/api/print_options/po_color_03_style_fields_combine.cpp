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
  std::cout << std::boolalpha;
  Style s{.fg = Color::Blue, .bg = Color::Black, .bold = true, .underline = true};
  std::cout << "empty default: " << Style{}.empty() << '\n';
  std::cout << "empty with fields set: " << s.empty() << '\n';
  std::cout << "equal to itself: " << (s == s) << '\n';
  std::cout << "different from default: " << !(s == Style{}) << '\n';
  return 0;
}
