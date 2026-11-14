#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  const pluss::Series losses = pluss::make_series({50.0, 100.0, 200.0, 500.0});
  const actuarial::LossTransform deductible =
      actuarial::operators::deductible(100.0);
  const actuarial::LossTransform limit = actuarial::operators::limit(150.0);
  const actuarial::LossTransform coinsurance =
      actuarial::operators::coinsurance(0.80);
  const actuarial::LossTransform layer =
      actuarial::operators::layer(100.0, 200.0, 0.50);
  const actuarial::LossMeasure mean = actuarial::operators::mean();

  out.writeln("losses =", losses.items<double>());
  out.writeln("deductible =", deductible(losses).items<double>());
  out.writeln("limit =", limit(losses).items<double>());
  out.writeln("coinsurance =", coinsurance(losses).items<double>());
  out.writeln("layer =", layer(losses).items<double>());
  out.writeln("layer mean =", mean(layer(losses)).item<double>());
}
