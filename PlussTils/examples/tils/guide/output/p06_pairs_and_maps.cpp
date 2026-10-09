// A feature-flag report. Each flag is a key and a value, shown as a map with a chosen key-value
// separator and brackets, and a pair shows the same shape for one comparison.
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <map>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  std::map<std::string, int> rollout{{"beta-search", 25}, {"new-checkout", 100}, {"dark-mode", 0}};

  out.writeln("rollout:", rollout, PrintOptions{.sort_keys = true, .kv_sep = " = ", .map_brackets = std::array<std::string, 2>{"{ ", " }"}});
  out.writeln("rollout list:", rollout, PrintOptions{.sort_keys = true, .kv_sep = ": ", .item_sep = " | "});
  out.writeln("before and after:", std::pair{25, 60}, PrintOptions{.pair_style = PairStyle::Arrow});
  out.writeln("before and after:", std::pair{25, 60}, PrintOptions{.pair_style = PairStyle::Tuple});
  return 0;
}
