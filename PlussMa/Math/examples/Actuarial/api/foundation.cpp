#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  const actuarial::Operator<double(double)> net =
      actuarial::make_operator<double(double)>(
          "net_after_tax", [](double gross) { return gross * 0.75; });

  out.writeln("key =", net.key());
  out.writeln("defined =", net.defined());
  out.writeln("net(120) =", net(120.0));
}
