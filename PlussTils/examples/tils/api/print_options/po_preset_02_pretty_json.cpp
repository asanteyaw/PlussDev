#include <pluss/tils/auxiliary/output.h>

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
  out.writeln(std::map<std::string, std::vector<int>>{{"b", {3, 4}}, {"a", {1, 2}}}, PrintOptions::pretty_json());
  return 0;
}
