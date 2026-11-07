#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writef("[{:>6}] [{:<6}] [{:06.2f}]", "right", "left", 3.14159);
  out.endln();
  return 0;
}
