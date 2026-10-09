#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Right, .width = 8, .fill = '.'});
  return 0;
}
