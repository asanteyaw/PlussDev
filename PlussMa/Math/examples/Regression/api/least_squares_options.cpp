#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace regression = pluss::ma::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  const pluss::Series y = pluss::make_series({2.1, 4.0, 8.0, 8.1, 10.0, 12.2});

  const regression::OLSFitResult plain = regression::ols_fit(y, x);

  regression::OLSOptions weighted_options;
  weighted_options.weights = pluss::make_series({1.0, 1.0, 4.0, 1.0, 1.0, 1.0});
  const regression::OLSFitResult weighted = regression::ols_fit(y, x, weighted_options);

  regression::OLSOptions origin_options;
  origin_options.add_intercept = false;
  const regression::OLSFitResult through_origin = regression::ols_fit(y, x, origin_options);

  out.writeln("== option effects ==");
  out.writeln("plain =", plain.coefficients.items<double>());
  out.writeln("weighted =", weighted.coefficients.items<double>());
  out.writeln("through origin =", through_origin.coefficients.items<double>());
  out.writeln("origin has intercept =", through_origin.has_intercept ? "true" : "false");
  out.writeln("origin coefficient names =", through_origin.coefficient_names);
}
