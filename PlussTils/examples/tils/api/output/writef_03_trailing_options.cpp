#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writef("value {}", 1234567.5, PrintOptions{}.fixed(2).grouped(','));
  out.endln();
  return 0;
}
