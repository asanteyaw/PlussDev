#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("TrueFalse:", true, false, PrintOptions{.bool_style = BoolStyle::TrueFalse});
  out.writeln("OneZero:", true, false, PrintOptions{.bool_style = BoolStyle::OneZero});
  out.writeln("YesNo:", true, false, PrintOptions{.bool_style = BoolStyle::YesNo});
  out.writeln("OnOff:", true, false, PrintOptions{.bool_style = BoolStyle::OnOff});
  return 0;
}
