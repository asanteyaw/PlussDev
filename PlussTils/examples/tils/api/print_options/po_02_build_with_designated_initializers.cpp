#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  const PrintOptions p{.notation = Notation::Fixed, .precision = 2, .grouping = ','};
  out.writeln("total:", 1234567.5, p);
  return 0;
}
