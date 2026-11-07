// Dumping a configuration and a lookup table for a support ticket. Containers print in a readable
// form, maps can be sorted by key, long lists can be capped, and multiline output is indented.
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  std::map<std::string, std::string> settings{
      {"timeout", "30s"}, {"region", "eu-west"}, {"retries", "3"}};
  std::vector<int> recent_codes{200, 200, 404, 500, 200, 503, 200, 200, 429, 200};

  out.writeln("settings:", settings, PrintOptions{.sort_keys = true}.pretty());
  out.writeln("recent codes, first four:", recent_codes, PrintOptions{}.capped(4));
  out.writeln("recent codes, one per line:", recent_codes,
              PrintOptions{.multiline = true, .indent = "    "});
  return 0;
}
