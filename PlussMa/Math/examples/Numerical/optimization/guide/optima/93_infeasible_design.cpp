// A design that cannot be built. The same kind of can, now with a catalogue limit on the sizes: the radius may not
// exceed 2 and the height may not exceed 20, while the can must hold 1000 cubic centimetres. The largest can that the limits allow
// holds pi * 2^2 * 20 = 251 cubic centimetres, so no point satisfies every requirement. The program shows what the
// solver reports instead of an answer (an exit flag and a violation that does not go away), then relaxes the height
// limit and solves again.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> rh(double r, double h) { return optim::pack_scalars({{"r", scalar(r)}, {"h", scalar(h)}}); }

void solve(const char* label, double max_height) {
  const double pi = 3.14159265358979323846, volume = 1000.0;
  auto area = [&](const Pack& p) { return 2.0 * pi * p.at("r").square() + 2.0 * pi * p.at("r") * p.at("h"); };
  const auto can = optim::constraints()
                       .lbs(rh(0.1, 0.1))
                       .ubs(rh(2.0, max_height))
                       .nl_ineq([&](const Pack& p) { return volume - pi * p.at("r").square() * p.at("h"); });
  const auto r = optim::optima(area, rh(1, 10), {}, can);
  std::cout << label << ": exit flag " << static_cast<int>(r.exit_flag) << ", max violation " << std::scientific << std::setprecision(2)
            << r.evaluation.max_constraint_violation << ", radius " << std::fixed << std::setprecision(4) << as_f64(r.params.at("r"))
            << ", height " << as_f64(r.params.at("h")) << "\n  " << r.message.substr(0, r.message.find(" diagnostic")) << "\n";
}

int main() {
  solve("height limit 20 ", 20.0);
  solve("height limit 400", 400.0);
  return 0;
}
