// A log of sensor readings. Small and large measurements switch to scientific notation, a general
// notation keeps significant digits, and missing or invalid readings print as named text.
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.writeln("electron mass (kg):", 9.109e-31, PrintOptions{.notation = Notation::Scientific, .precision = 3});
  out.writeln("sample rate (Hz):", 48000.0, PrintOptions{.notation = Notation::General, .precision = 4});
  out.writeln("pressure (Pa):", 101325.0, PrintOptions{.notation = Notation::General, .precision = 4});
  out.writeln("readings:", std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::infinity(),
              PrintOptions{.nan_text = "missing", .pos_inf_text = "overflow"});
  return 0;
}
