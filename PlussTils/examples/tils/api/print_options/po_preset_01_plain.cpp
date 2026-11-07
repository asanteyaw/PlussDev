#include <pluss/tils/kit/output.h>

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
  out.writeln(std::vector<int>{1, 2, 3}, std::map<std::string, int>{{"a", 1}}, PrintOptions::plain());
  return 0;
}
