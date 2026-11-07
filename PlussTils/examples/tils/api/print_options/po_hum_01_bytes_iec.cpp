#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("1536 bytes:", 1536, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("3 GiB:", 3221225472LL, PrintOptions{.humanize = Humanize::BytesIEC});
  return 0;
}
