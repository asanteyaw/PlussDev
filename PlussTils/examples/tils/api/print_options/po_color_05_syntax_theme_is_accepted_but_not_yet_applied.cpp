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
  std::ostringstream captured;
  OutputEngine cap{captured};
  SyntaxTheme theme;
  theme.number = Style{.fg = Color::Cyan};
  cap.writeln("value:", 42, PrintOptions{.color_mode = ColorMode::Always, .syntax = theme});
  // The struct is accepted and does not refuse to compile or run, but it does not yet color the number.
  std::string text = captured.str();
  std::cout << "no per-token escape for the number yet: " << (text.find("36m42") == std::string::npos) << '\n';
  return 0;
}
