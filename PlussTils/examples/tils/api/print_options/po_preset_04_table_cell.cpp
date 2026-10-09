#include <pluss/tils/aux/output.h>

#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("[{}]", "name", PrintOptions::table_cell(10));
  out.writelnf("[{}]", "an-overlong-name", PrintOptions::table_cell(10));
  return 0;
}
