#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  std::cout << "Red is not Blue: " << (Color::Red != Color::Blue) << '\n';
  std::cout << "Default is the fallback foreground: " << (Style{}.fg == Color::Default) << '\n';
  std::ostringstream captured;
  OutputEngine cap{captured};
  cap.writeln("warning", PrintOptions{.color_mode = ColorMode::Always, .style = Style{.fg = Color::BrightYellow}});
  std::string text = captured.str();
  for (char& c : text) if (c == '\x1b') c = '!';
  std::cout << text;
  return 0;
}
