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
  std::ostringstream captured;
  OutputEngine cap{captured};
  cap.writeln("green", PrintOptions{.color_mode = ColorMode::Always}.colored(Style{.fg = Color::Green}));
  cap.writeln(std::vector<std::string>{"q"}, PrintOptions{}.quoted());
  cap.writelnf("{}", "shout", PrintOptions{}.upcase());
  std::string text = captured.str();
  for (char& c : text) if (c == '\x1b') c = '!';
  std::cout << text;
  return 0;
}
