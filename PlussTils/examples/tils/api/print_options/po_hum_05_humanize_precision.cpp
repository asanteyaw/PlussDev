#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("default precision:", 1536, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("precision 2:", 1536, PrintOptions{.humanize = Humanize::BytesIEC, .humanize_precision = 2});
  return 0;
}
