#include <pluss/tils/kit/output.h>

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
  out.writeln(1, 2, 3, PrintOptions{}.with_sep(" / "));
  out.writeln("x", PrintOptions{}.with_end("!"));
  return 0;
}
