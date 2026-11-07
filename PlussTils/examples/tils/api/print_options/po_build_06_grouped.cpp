#include <pluss/tils/auxiliary/output.h>

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
  out.writeln(1234567, PrintOptions{}.grouped());
  out.writeln(12345678, PrintOptions{}.grouped('_', 2));
  return 0;
}
