#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.write("no newline here, ");
  out.write("the rest continues");
  out.endln();
  return 0;
}
