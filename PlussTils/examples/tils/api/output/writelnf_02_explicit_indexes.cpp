#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("{1} before {0}", "first", "second");
  out.writelnf("{0} repeated: {0}", "again");
  return 0;
}
