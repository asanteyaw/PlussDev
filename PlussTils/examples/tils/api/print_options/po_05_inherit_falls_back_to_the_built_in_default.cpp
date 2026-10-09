#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  // align is left at Inherit, so a width still pads, using the built-in default alignment.
  out.writelnf("[{}]", "hi", PrintOptions{.width = 6});
  return 0;
}
