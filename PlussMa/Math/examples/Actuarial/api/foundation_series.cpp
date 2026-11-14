#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  const actuarial::Operator<pluss::Series(const pluss::Series &)> load =
      actuarial::make_operator<pluss::Series(const pluss::Series &)>(
          "expense_load",
          [](const pluss::Series &premium) { return premium * 1.15; });
  const pluss::Series premium = pluss::make_series({100.0, 125.0, 160.0});

  out.writeln("key =", load.key());
  out.writeln("loaded premium =", load(premium).items<double>());
}
