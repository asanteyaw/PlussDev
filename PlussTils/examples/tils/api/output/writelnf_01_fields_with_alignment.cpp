#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("{:<8}{:>6}", "north", 12);
  out.writelnf("{:<8}{:>6}", "east", 140);
  return 0;
}
