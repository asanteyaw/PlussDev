#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("{}", "a-very-long-service-name", PrintOptions{.truncate = 12, .ellipsis = "~"});
  return 0;
}
