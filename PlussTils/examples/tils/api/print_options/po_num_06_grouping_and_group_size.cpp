#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("comma:", 1234567, PrintOptions{.grouping = ','});
  out.writeln("underscore:", 1234567, PrintOptions{.grouping = '_'});
  out.writeln("group size 4:", 123456789, PrintOptions{.grouping = ',', .group_size = 4});
  return 0;
}
