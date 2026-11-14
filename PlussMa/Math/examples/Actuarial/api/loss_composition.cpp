#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  const pluss::Series losses = pluss::make_series({80.0, 150.0, 300.0, 700.0});
  const actuarial::LossTransform excess =
      actuarial::operators::deductible(100.0);
  const actuarial::LossTransform cap = actuarial::operators::limit(250.0);
  const actuarial::LossTransform share =
      actuarial::operators::coinsurance(0.80);
  const actuarial::LossMeasure mean = actuarial::operators::mean();
  const pluss::Series retained = share(cap(excess(losses)));

  out.writeln("retained =", retained.items<double>());
  out.writeln("mean retained =", mean(retained).item<double>());
}
