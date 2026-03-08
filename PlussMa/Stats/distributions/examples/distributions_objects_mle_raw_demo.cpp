#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using namespace pluss::subsp::stat::distr;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series xn = pluss::make_series({1.0, 2.0, 3.0, 4.0}, f64);
  const Series xe = pluss::make_series({0.5, 1.0, 1.5, 2.0}, f64);
  const Series xp = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0}, f64);
  const Series xb = pluss::make_series({0.0, 1.0, 1.0, 0.0, 1.0}, f64);
  const Series xbin = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0}, f64);
  const Series xg = pluss::make_series({1.0, 2.0, 3.0, 4.0}, f64);
  const Series xl = pluss::make_series({1.0, 2.0, 4.0, 8.0}, f64);
  const Series xu = pluss::make_series({2.0, 3.0, 5.0, 7.0}, f64);
  const Series xr = pluss::make_series({1.0, 2.0, 3.0, 4.0}, f64);

  std::cout << "Raw MLE object usage demo\n";

  const auto nfit = Normal{0.0, 1.0}.MLE(xn);
  std::cout << "\nNormal\n";
  std::cout << "mean=" << nfit.mean << " stddev=" << nfit.stddev << "\n";
  std::cout << "loglik=" << nfit.loglik(xn) << "\n";
  std::cout << "nll=" << nfit.nll(xn) << "\n";
  std::cout << "score=" << nfit.score(xn) << "\n";
  std::cout << "fisher_inf=" << nfit.fisher_inf(xn) << "\n";
  std::cout << "observed_inf=" << nfit.observed_inf(xn) << "\n";
  std::cout << "vcov=" << nfit.vcov(xn) << "\n";
  std::cout << "stderr=" << nfit.stderr(xn) << "\n";
  std::cout << "confint=" << nfit.confint(xn) << "\n";

  const auto efit = Exponential{1.0}.MLE(xe);
  std::cout << "\nExponential\n";
  std::cout << "rate=" << efit.rate << "\n";
  std::cout << "score=" << efit.score(xe) << "\n";
  std::cout << "fisher_inf=" << efit.fisher_inf(xe) << "\n";
  std::cout << "vcov=" << efit.vcov(xe) << "\n";
  std::cout << "stderr=" << efit.stderr(xe) << "\n";
  std::cout << "confint=" << efit.confint(xe) << "\n";

  const auto pfit = Poisson{1.0}.MLE(xp);
  std::cout << "\nPoisson\n";
  std::cout << "lambda=" << pfit.lambda << "\n";
  std::cout << "score=" << pfit.score(xp) << "\n";
  std::cout << "fisher_inf=" << pfit.fisher_inf(xp) << "\n";
  std::cout << "vcov=" << pfit.vcov(xp) << "\n";
  std::cout << "stderr=" << pfit.stderr(xp) << "\n";
  std::cout << "confint=" << pfit.confint(xp) << "\n";

  const auto bfit = Bernoulli{0.5}.MLE(xb);
  std::cout << "\nBernoulli\n";
  std::cout << "p=" << bfit.p << "\n";
  std::cout << "score=" << bfit.score(xb) << "\n";
  std::cout << "fisher_inf=" << bfit.fisher_inf(xb) << "\n";
  std::cout << "vcov=" << bfit.vcov(xb) << "\n";
  std::cout << "stderr=" << bfit.stderr(xb) << "\n";
  std::cout << "confint=" << bfit.confint(xb) << "\n";

  const auto binfit = Binomial{4, 0.5}.MLE(xbin);
  std::cout << "\nBinomial\n";
  std::cout << "count=" << binfit.count << " p=" << binfit.p << "\n";
  std::cout << "score=" << binfit.score(xbin) << "\n";
  std::cout << "fisher_inf=" << binfit.fisher_inf(xbin) << "\n";
  std::cout << "vcov=" << binfit.vcov(xbin) << "\n";
  std::cout << "stderr=" << binfit.stderr(xbin) << "\n";
  std::cout << "confint=" << binfit.confint(xbin) << "\n";

  const auto gfit = Geometric{0.5}.MLE(xg);
  std::cout << "\nGeometric\n";
  std::cout << "p=" << gfit.p << "\n";
  std::cout << "score=" << gfit.score(xg) << "\n";
  std::cout << "fisher_inf=" << gfit.fisher_inf(xg) << "\n";
  std::cout << "vcov=" << gfit.vcov(xg) << "\n";
  std::cout << "stderr=" << gfit.stderr(xg) << "\n";
  std::cout << "confint=" << gfit.confint(xg) << "\n";

  const auto lfit = Lognormal{0.0, 1.0}.MLE(xl);
  std::cout << "\nLognormal\n";
  std::cout << "mean_log=" << lfit.mean_log << " stddev_log=" << lfit.stddev_log << "\n";
  std::cout << "score=" << lfit.score(xl) << "\n";
  std::cout << "fisher_inf=" << lfit.fisher_inf(xl) << "\n";
  std::cout << "confint=" << lfit.confint(xl) << "\n";

  const auto ufit = Uniform{0.0, 1.0}.MLE(xu);
  std::cout << "\nUniform\n";
  std::cout << "low=" << ufit.low << " high=" << ufit.high << "\n";
  std::cout << "score=" << ufit.score(xu) << "\n";
  std::cout << "fisher_inf=" << ufit.fisher_inf(xu) << "\n";
  std::cout << "confint=" << ufit.confint(xu) << "\n";

  const auto rfit = Rayleigh{1.0}.MLE(xr);
  std::cout << "\nRayleigh\n";
  std::cout << "sigma=" << rfit.sigma << "\n";
  std::cout << "score=" << rfit.score(xr) << "\n";
  std::cout << "fisher_inf=" << rfit.fisher_inf(xr) << "\n";
  std::cout << "vcov=" << rfit.vcov(xr) << "\n";
  std::cout << "stderr=" << rfit.stderr(xr) << "\n";
  std::cout << "confint=" << rfit.confint(xr) << "\n";

  return 0;
}
