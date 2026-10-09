#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("Auto:", 3.5, PrintOptions{.notation = Notation::Auto});
  out.writeln("Fixed:", 3.14159, PrintOptions{.notation = Notation::Fixed, .precision = 2});
  out.writeln("Scientific:", 123456.789, PrintOptions{.notation = Notation::Scientific, .precision = 2});
  out.writeln("General:", 123456.789, PrintOptions{.notation = Notation::General, .precision = 4});
  out.writeln("Hex:", 255, PrintOptions{.notation = Notation::Hex});
  out.writeln("Percent:", 0.1275, PrintOptions{.notation = Notation::Percent, .precision = 1});
  out.writeln("Metric:", 12345.0, PrintOptions{.notation = Notation::Metric});
  out.writeln("Binary:", 10, PrintOptions{.notation = Notation::Binary});
  out.writeln("Octal:", 64, PrintOptions{.notation = Notation::Octal});
  return 0;
}
