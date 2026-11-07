#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.enable_levels(false);
  out.warn("untagged");
  out.enable_levels(true);
  out.warn("tagged again");
  return 0;
}
