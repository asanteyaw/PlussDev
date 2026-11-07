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
  out.writeln("quiet", PrintOptions{.color_mode = ColorMode::Never, .style = Style{.fg = Color::Red}});
  return 0;
}
