#include <pluss/tils/aux/output.h>

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
  Style rgb{};
  rgb.fg = Color::Red;  // overridden by fg_rgb below
  rgb.fg_rgb = std::array<std::uint8_t, 3>{255, 128, 0};
  cap.writeln("orange", PrintOptions{.color_mode = ColorMode::Always, .style = rgb});
  std::string text = captured.str();
  std::cout << "escape names the 24-bit color: " << (text.find("255;128;0") != std::string::npos) << '\n';
  return 0;
}
