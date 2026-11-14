#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace regression = pluss::ma::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const pluss::Series x = pluss::make_series({
      {1.0, 2.0}, {2.0, 1.0}, {3.0, 0.0}, {4.0, 2.0},
      {5.0, 1.0}, {6.0, 3.0}, {7.0, 4.0}, {8.0, 5.0},
  });
  const pluss::Series y = pluss::make_series({0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9});

  regression::OLSOptions options;
  options.response_name = "target";
  options.predictor_names = {"feat1", "feat2"};

  const regression::OLSFitResult fit = regression::ols_fit(y, x, options);
  out.writeln("== fit ==");
  out.writeln("names =", fit.coefficient_names);
  out.writeln("coefficients =", fit.coefficients.items<double>());
  out.writeln("standard errors =", fit.std_errors.items<double>());
  out.writeln("nobs =", fit.nobs, ", df_resid =", fit.df_resid);
  out.writeln("r-squared =", fit.rsquared, ", adjusted =", fit.adj_rsquared);
  out.writeln();

  const pluss::Series future = pluss::make_series({{9.0, 6.0}, {10.0, 7.0}});
  const pluss::Series predicted = regression::ols_predict(fit, future);
  out.writeln("== prediction ==");
  out.writeln("values =", predicted.items<double>());
}
