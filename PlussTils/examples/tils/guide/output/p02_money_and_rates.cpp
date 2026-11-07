// A dashboard of money and rates. Revenue has grouping and two decimals, a conversion rate is a
// percentage, and a change from the last period always shows its sign.
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  out.writeln("revenue:", 1234567.5, PrintOptions{}.fixed(2).grouped(','));
  out.writeln("conversion:", 0.0427, PrintOptions{.notation = Notation::Percent, .precision = 2});
  out.writeln("change (up):", 12.5, PrintOptions{.sign = Sign::Always, .notation = Notation::Fixed, .precision = 1});
  out.writeln("change (down):", -3.25, PrintOptions{.sign = Sign::Always, .notation = Notation::Fixed, .precision = 2});
  out.writeln("ticket count:", 1234567, PrintOptions{.grouping = ','});
  out.writeln("ticket count with spaces:", 1234567, PrintOptions{.grouping = ' '});
  out.writeln("european decimal:", 3.5, PrintOptions{.notation = Notation::Fixed, .precision = 2, .decimal_point = ','});
  return 0;
}
