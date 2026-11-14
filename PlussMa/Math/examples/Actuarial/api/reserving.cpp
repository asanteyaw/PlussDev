#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  const actuarial::Triangle triangle = actuarial::triangle::cumulative(
      pluss::make_series(
          {{100.0, 150.0, 180.0}, {120.0, 180.0, 0.0}, {130.0, 0.0, 0.0}}),
      pluss::make_series(
          {{true, true, true}, {true, true, false}, {true, false, false}}),
      {"2023", "2024", "2025"}, {"12", "24", "36"});

  const actuarial::Reserving chain_ladder =
      actuarial::operators::chain_ladder();
  const actuarial::ReserveResult chain = chain_ladder(triangle);
  out.writeln("origins =", triangle.origins());
  out.writeln("development =", triangle.development());
  out.writeln("age-to-age =", chain.age_to_age.items<double>());
  out.writeln("cdf =", chain.cdf.items<double>());
  out.writeln("ultimate =", chain.ultimate.items<double>());
  out.writeln("reserve =", chain.reserve.items<double>());

  const actuarial::Reserving bf = actuarial::operators::bornhuetter_ferguson(
      pluss::make_series({200.0, 220.0, 250.0}),
      pluss::make_series({0.90, 0.70, 0.50}));
  const actuarial::ReserveResult bf_result = bf(triangle);
  out.writeln("BF ultimate =", bf_result.ultimate.items<double>());
  out.writeln("BF reserve =", bf_result.reserve.items<double>());
}
