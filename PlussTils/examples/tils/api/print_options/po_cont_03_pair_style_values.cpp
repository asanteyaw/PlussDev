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
  out.writeln("Tuple:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Tuple});
  out.writeln("Colon:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Colon});
  out.writeln("Arrow:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Arrow});
  out.writeln("Equals:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Equals});
  return 0;
}
