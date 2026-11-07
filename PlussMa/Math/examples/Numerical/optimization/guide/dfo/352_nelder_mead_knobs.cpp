// The knobs of `NelderMeadConfig`, one at a time, on the same 4-D quadratic with an off-axis valley. Each row changes a
// single setting from the default and shows what it does to the work (evaluations) and the accuracy (minimum reached).
//   position_tolerance, value_tolerance   stop when the simplex is this small AND its values agree to this
//   simplex_delta                         relative size of the starting simplex (default 5%)
//   reflection / expansion / contraction / shrink   the four step coefficients (defaults 1, 2, 0.5, 0.5)
//   max_evaluations                       budget
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::vector<std::pair<const char*, dfo::NelderMeadConfig>> cases;
  dfo::NelderMeadConfig base;
  cases.emplace_back("defaults", base);
  auto tight = base; tight.position_tolerance = 1e-8; tight.value_tolerance = 1e-8;
  cases.emplace_back("tolerances 1e-8", tight);
  auto loose = base; loose.position_tolerance = 1e-2; loose.value_tolerance = 1e-2;
  cases.emplace_back("tolerances 1e-2", loose);
  auto big = base; big.simplex_delta = 0.5;
  cases.emplace_back("simplex_delta 0.5", big);
  auto tiny = base; tiny.simplex_delta = 0.001;
  cases.emplace_back("simplex_delta 0.001", tiny);
  auto bold = base; bold.expansion = 3.0;
  cases.emplace_back("expansion 3", bold);
  auto timid = base; timid.contraction = 0.25; timid.shrink = 0.25;
  cases.emplace_back("contraction, shrink 0.25", timid);
  auto capped = base; capped.max_evaluations = 100;
  cases.emplace_back("max_evaluations 100", capped);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    auto f = [](const pluss::Series& x) {
      const auto d = x - vec({1.0, -1.0, 2.0, 0.5});
      const auto s = d.select(0, 0) + d.select(0, 1) + d.select(0, 2) + d.select(0, 3);
      return d.square().sum() + s.square() * 0.8;
    };
    const auto o = dfo::nelder_mead_minimizer(f, vec({3.0, 2.0, -1.0, 1.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
