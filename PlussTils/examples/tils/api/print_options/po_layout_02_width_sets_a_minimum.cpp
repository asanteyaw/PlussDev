#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("[{}]", "ok", PrintOptions{.width = 8});
  out.writelnf("[{}]", "already long enough", PrintOptions{.width = 8});
  return 0;
}
