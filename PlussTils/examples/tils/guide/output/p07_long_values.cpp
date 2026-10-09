// A log line that quotes a long history. Lists are capped with a count of what was left out, and
// nested lists are cut at a depth so one value cannot fill the terminal.
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  std::vector<int> history{4, 8, 15, 16, 23, 42, 8, 4, 15, 16};
  std::vector<std::vector<int>> grid{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

  out.writeln("last ten values:", history, PrintOptions{.max_items = 4});
  out.writeln("last ten, with a count:", history, PrintOptions{.max_items = 4, .more_verbose = true});
  out.writeln("grid, nested:", grid);
  out.writeln("grid, top level only:", grid, PrintOptions{.max_depth = 1});
  return 0;
}
