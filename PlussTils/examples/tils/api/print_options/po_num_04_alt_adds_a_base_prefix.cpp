#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("hex without alt:", 255, PrintOptions{.notation = Notation::Hex});
  out.writeln("hex with alt:", 255, PrintOptions{.notation = Notation::Hex, .alt = true});
  out.writeln("octal with alt:", 64, PrintOptions{.notation = Notation::Octal, .alt = true});
  out.writeln("binary with alt:", 10, PrintOptions{.notation = Notation::Binary, .alt = true});
  return 0;
}
