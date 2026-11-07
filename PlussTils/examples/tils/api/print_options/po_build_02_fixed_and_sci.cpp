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
  out.writeln(3.14159, PrintOptions{}.fixed(2));
  out.writeln(98765.0, PrintOptions{}.sci(1));
  return 0;
}
