#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("NegativeOnly:", 5, -5, PrintOptions{.sign = Sign::NegativeOnly});
  out.writeln("Always:", 5, -5, PrintOptions{.sign = Sign::Always});
  out.writeln("SpaceForPositive:", 5, -5, PrintOptions{.sign = Sign::SpaceForPositive});
  return 0;
}
