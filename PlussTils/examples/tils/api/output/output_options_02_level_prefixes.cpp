#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.level_prefixes = {"trc", "dbg", "inf", "wrn", "err"};
  OutputEngine out{std::cout, options};
  out.debug("short prefixes");
  out.error("short prefixes again");
  return 0;
}
