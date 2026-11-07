#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("1500 bytes:", 1500, PrintOptions{.humanize = Humanize::BytesSI});
  out.writeln("2.5 MB:", 2500000LL, PrintOptions{.humanize = Humanize::BytesSI});
  return 0;
}
