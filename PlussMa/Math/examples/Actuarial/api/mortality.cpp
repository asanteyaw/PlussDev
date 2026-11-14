#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const actuarial::Mortality from_qx =
      actuarial::mortality::from_qx(pluss::make_series({0.10, 0.20, 1.00}));
  const actuarial::Mortality from_px =
      actuarial::mortality::from_px(pluss::make_series({0.90, 0.80, 0.00}));
  const actuarial::Mortality from_force =
      actuarial::mortality::from_force(pluss::make_series({0.10, 0.20, 0.30}));
  const actuarial::Mortality from_lx = actuarial::mortality::from_lx(
      pluss::make_series({1000.0, 900.0, 720.0, 0.0}));

  out.writeln("qx =", from_qx.qx().items<double>());
  out.writeln("px =", from_qx.px().items<double>());
  out.writeln("survival =", from_qx.survival().items<double>());
  out.writeln("lifetime pmf =", from_qx.lifetime_pmf().items<double>());
  out.writeln("from px agrees =", from_px.qx().allclose(from_qx.qx()));
  out.writeln("from lx agrees =", from_lx.qx().allclose(from_qx.qx()));
  out.writeln("from force qx =", from_force.qx().items<double>());

  const actuarial::MortalityModel gompertz =
      actuarial::mortality::gompertz(0.00002, 1.10);
  const actuarial::MortalityModel makeham =
      actuarial::mortality::makeham(0.0005, 0.00002, 1.10);
  const pluss::Series ages = pluss::make_series({40.0, 50.0, 60.0});
  out.writeln("gompertz qx =", gompertz(ages).qx().items<double>());
  out.writeln("makeham qx =", makeham(ages).qx().items<double>());

  const actuarial::Mortality selected = actuarial::mortality::select_ultimate(
      pluss::make_series({0.01, 0.02}), pluss::make_series({0.03, 0.04, 1.00}),
      4);
  out.writeln("select-ultimate qx =", selected.qx().items<double>());
}
