// Colored status lines for a terminal tool. Color is forced on here so the escape sequences show
// up in the captured output. In a real terminal, color_mode Auto colors only when the output is a
// TTY, which is the usual choice.
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

// Replaces the escape byte with the text ESC, so the sequences are readable in the document.
std::string visible(std::string text) {
  std::string out;
  for (char c : text) {
    if (c == '\x1b') out += "ESC";
    else out += c;
  }
  return out;
}

int main() {
  std::ostringstream captured;
  OutputEngine out{captured};
  out.writeln("build:", "passed", PrintOptions{.color_mode = ColorMode::Always,
                                               .style = Style{.fg = Color::Green, .bold = true}});
  out.writeln("deploy:", "blocked", PrintOptions{.color_mode = ColorMode::Always,
                                                 .style = Style{.fg = Color::Red}});
  out.writeln("notes:", "plain text", PrintOptions{.color_mode = ColorMode::Never});
  std::cout << visible(captured.str());
  return 0;
}
