#include <pluss/pluss.h>

#include <iostream>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using namespace pluss::subsp::stat::distr;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series enum_draw = pluss::distr::sample(dst::Normal, {0.0, 1.0}, {6}, f64, 20260309ULL);

  const Series xn = pluss::make_series({-1.2, -0.2, 0.1, 0.8, 1.4, 2.0}, f64);
  const Series xe = pluss::make_series({0.3, 0.6, 1.1, 1.7, 2.4, 3.2}, f64);
  const Series xp = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 2.0, 1.0}, f64);
  const Series xb = pluss::make_series({0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0}, f64);
  const Series xbin = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 2.0, 1.0}, f64);
  const Series xg = pluss::make_series({1.0, 2.0, 3.0, 4.0, 2.0, 1.0}, f64);
  const Series xl = pluss::make_series({1.0, 2.0, 4.0, 8.0, 16.0}, f64);
  const Series xu = pluss::make_series({2.2, 2.8, 3.5, 4.1, 5.0, 5.7}, f64);
  const Series xr = pluss::make_series({1.0, 2.0, 3.0, 4.0, 2.5, 1.5}, f64);
  const Series xgamma = pluss::make_series({0.8, 1.1, 1.9, 2.7, 3.6, 4.9}, f64);
  const Series xbeta = pluss::make_series({0.10, 0.22, 0.35, 0.41, 0.57, 0.68, 0.81}, f64);
  const Series xnb = pluss::make_series({0.0, 0.0, 1.0, 2.0, 4.0, 7.0, 3.0, 6.0}, f64);
  const Series xheavy = pluss::make_series({-2.4, -1.1, -0.3, 0.2, 0.9, 1.8, 3.1}, f64);
  const Series xmix = pluss::make_series({-3.0, -2.5, -2.1, -1.4, 0.9, 1.2, 1.8, 2.5, 3.0}, f64);

  std::cout << "Raw MLE object usage demo (closed form + approximation families)\n";
  std::cout << "enum sample(dst::Normal, ...): " << enum_draw << "\n";

  const auto nfit = Normal{0.0, 1.0}.MLE(xn);
  std::cout << "\nNormal\n";
  std::cout << "mean=" << nfit.mean << " stddev=" << nfit.stddev << "\n";
  std::cout << "loglik=" << nfit.loglik(xn) << "\n";
  std::cout << "loglik_per_obs=" << nfit.loglik_per_obs(xn) << "\n";
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
  std::cout << "loglik=" << efit.loglik(xe) << "\n";
  std::cout << "loglik_per_obs=" << efit.loglik_per_obs(xe) << "\n";
  std::cout << "nll=" << efit.nll(xe) << "\n";
  std::cout << "score=" << efit.score(xe) << "\n";
  std::cout << "fisher_inf=" << efit.fisher_inf(xe) << "\n";
  std::cout << "observed_inf=" << efit.observed_inf(xe) << "\n";
  std::cout << "vcov=" << efit.vcov(xe) << "\n";
  std::cout << "stderr=" << efit.stderr(xe) << "\n";
  std::cout << "confint=" << efit.confint(xe) << "\n";

  const auto pfit = Poisson{1.0}.MLE(xp);
  std::cout << "\nPoisson\n";
  std::cout << "lambda=" << pfit.lambda << "\n";
  std::cout << "loglik=" << pfit.loglik(xp) << "\n";
  std::cout << "loglik_per_obs=" << pfit.loglik_per_obs(xp) << "\n";
  std::cout << "nll=" << pfit.nll(xp) << "\n";
  std::cout << "score=" << pfit.score(xp) << "\n";
  std::cout << "fisher_inf=" << pfit.fisher_inf(xp) << "\n";
  std::cout << "observed_inf=" << pfit.observed_inf(xp) << "\n";
  std::cout << "vcov=" << pfit.vcov(xp) << "\n";
  std::cout << "stderr=" << pfit.stderr(xp) << "\n";
  std::cout << "confint=" << pfit.confint(xp) << "\n";

  const auto bfit = Bernoulli{0.5}.MLE(xb);
  std::cout << "\nBernoulli\n";
  std::cout << "p=" << bfit.p << "\n";
  std::cout << "loglik=" << bfit.loglik(xb) << "\n";
  std::cout << "loglik_per_obs=" << bfit.loglik_per_obs(xb) << "\n";
  std::cout << "nll=" << bfit.nll(xb) << "\n";
  std::cout << "score=" << bfit.score(xb) << "\n";
  std::cout << "fisher_inf=" << bfit.fisher_inf(xb) << "\n";
  std::cout << "observed_inf=" << bfit.observed_inf(xb) << "\n";
  std::cout << "vcov=" << bfit.vcov(xb) << "\n";
  std::cout << "stderr=" << bfit.stderr(xb) << "\n";
  std::cout << "confint=" << bfit.confint(xb) << "\n";

  const auto binfit = Binomial{4, 0.5}.MLE(xbin);
  std::cout << "\nBinomial\n";
  std::cout << "count=" << binfit.count << " p=" << binfit.p << "\n";
  std::cout << "loglik=" << binfit.loglik(xbin) << "\n";
  std::cout << "loglik_per_obs=" << binfit.loglik_per_obs(xbin) << "\n";
  std::cout << "nll=" << binfit.nll(xbin) << "\n";
  std::cout << "score=" << binfit.score(xbin) << "\n";
  std::cout << "fisher_inf=" << binfit.fisher_inf(xbin) << "\n";
  std::cout << "observed_inf=" << binfit.observed_inf(xbin) << "\n";
  std::cout << "vcov=" << binfit.vcov(xbin) << "\n";
  std::cout << "stderr=" << binfit.stderr(xbin) << "\n";
  std::cout << "confint=" << binfit.confint(xbin) << "\n";

  const auto gmf = Geometric{0.5}.MLE(xg);
  std::cout << "\nGeometric\n";
  std::cout << "p=" << gmf.p << "\n";
  std::cout << "loglik=" << gmf.loglik(xg) << "\n";
  std::cout << "loglik_per_obs=" << gmf.loglik_per_obs(xg) << "\n";
  std::cout << "nll=" << gmf.nll(xg) << "\n";
  std::cout << "score=" << gmf.score(xg) << "\n";
  std::cout << "fisher_inf=" << gmf.fisher_inf(xg) << "\n";
  std::cout << "observed_inf=" << gmf.observed_inf(xg) << "\n";
  std::cout << "vcov=" << gmf.vcov(xg) << "\n";
  std::cout << "stderr=" << gmf.stderr(xg) << "\n";
  std::cout << "confint=" << gmf.confint(xg) << "\n";

  const auto lfit = Lognormal{0.0, 1.0}.MLE(xl);
  std::cout << "\nLognormal\n";
  std::cout << "mean_log=" << lfit.mean_log << " stddev_log=" << lfit.stddev_log << "\n";
  std::cout << "loglik=" << lfit.loglik(xl) << "\n";
  std::cout << "loglik_per_obs=" << lfit.loglik_per_obs(xl) << "\n";
  std::cout << "nll=" << lfit.nll(xl) << "\n";
  std::cout << "score=" << lfit.score(xl) << "\n";
  std::cout << "fisher_inf=" << lfit.fisher_inf(xl) << "\n";
  std::cout << "observed_inf=" << lfit.observed_inf(xl) << "\n";
  std::cout << "vcov=" << lfit.vcov(xl) << "\n";
  std::cout << "stderr=" << lfit.stderr(xl) << "\n";
  std::cout << "confint=" << lfit.confint(xl) << "\n";

  const auto ufit = Uniform{0.0, 1.0}.MLE(xu);
  std::cout << "\nUniform\n";
  std::cout << "low=" << ufit.low << " high=" << ufit.high << "\n";
  std::cout << "loglik=" << ufit.loglik(xu) << "\n";
  std::cout << "loglik_per_obs=" << ufit.loglik_per_obs(xu) << "\n";
  std::cout << "nll=" << ufit.nll(xu) << "\n";
  std::cout << "score=" << ufit.score(xu) << "\n";
  std::cout << "fisher_inf=" << ufit.fisher_inf(xu) << "\n";
  std::cout << "observed_inf=" << ufit.observed_inf(xu) << "\n";
  std::cout << "vcov=" << ufit.vcov(xu) << "\n";
  std::cout << "stderr=" << ufit.stderr(xu) << "\n";
  std::cout << "confint=" << ufit.confint(xu) << "\n";

  const auto rfit = Rayleigh{1.0}.MLE(xr);
  std::cout << "\nRayleigh\n";
  std::cout << "sigma=" << rfit.sigma << "\n";
  std::cout << "loglik=" << rfit.loglik(xr) << "\n";
  std::cout << "loglik_per_obs=" << rfit.loglik_per_obs(xr) << "\n";
  std::cout << "nll=" << rfit.nll(xr) << "\n";
  std::cout << "score=" << rfit.score(xr) << "\n";
  std::cout << "fisher_inf=" << rfit.fisher_inf(xr) << "\n";
  std::cout << "observed_inf=" << rfit.observed_inf(xr) << "\n";
  std::cout << "vcov=" << rfit.vcov(xr) << "\n";
  std::cout << "stderr=" << rfit.stderr(xr) << "\n";
  std::cout << "confint=" << rfit.confint(xr) << "\n";

  const auto gfit = Gamma{2.0, 1.0, 0.0}.MLE(xgamma);
  std::cout << "\nGamma\n";
  std::cout << "shape=" << gfit.shape << " scale=" << gfit.scale << " location=" << gfit.location << "\n";
  std::cout << "loglik=" << gfit.loglik(xgamma) << "\n";
  std::cout << "loglik_per_obs=" << gfit.loglik_per_obs(xgamma) << "\n";
  std::cout << "nll=" << gfit.nll(xgamma) << "\n";
  std::cout << "score=" << gfit.score(xgamma) << "\n";
  std::cout << "fisher_inf=" << gfit.fisher_inf(xgamma) << "\n";
  std::cout << "observed_inf=" << gfit.observed_inf(xgamma) << "\n";
  std::cout << "vcov=" << gfit.vcov(xgamma) << "\n";
  std::cout << "stderr=" << gfit.stderr(xgamma) << "\n";
  std::cout << "confint=" << gfit.confint(xgamma) << "\n";

  const auto betafit = Beta{0.0, 1.0, 2.0, 3.0}.MLE(xbeta);
  std::cout << "\nBeta\n";
  std::cout << "low=" << betafit.low << " high=" << betafit.high
            << " alpha=" << betafit.alpha << " beta=" << betafit.beta << "\n";
  std::cout << "loglik=" << betafit.loglik(xbeta) << "\n";
  std::cout << "loglik_per_obs=" << betafit.loglik_per_obs(xbeta) << "\n";
  std::cout << "nll=" << betafit.nll(xbeta) << "\n";
  std::cout << "score=" << betafit.score(xbeta) << "\n";
  std::cout << "fisher_inf=" << betafit.fisher_inf(xbeta) << "\n";
  std::cout << "observed_inf=" << betafit.observed_inf(xbeta) << "\n";
  std::cout << "vcov=" << betafit.vcov(xbeta) << "\n";
  std::cout << "stderr=" << betafit.stderr(xbeta) << "\n";
  std::cout << "confint=" << betafit.confint(xbeta) << "\n";

  const auto nbfit = NegativeBinomial{3.0, 0.5}.MLE(xnb);
  std::cout << "\nNegativeBinomial\n";
  std::cout << "r=" << nbfit.r << " p=" << nbfit.p << "\n";
  std::cout << "loglik=" << nbfit.loglik(xnb) << "\n";
  std::cout << "loglik_per_obs=" << nbfit.loglik_per_obs(xnb) << "\n";
  std::cout << "nll=" << nbfit.nll(xnb) << "\n";
  std::cout << "score=" << nbfit.score(xnb) << "\n";
  std::cout << "fisher_inf=" << nbfit.fisher_inf(xnb) << "\n";
  std::cout << "observed_inf=" << nbfit.observed_inf(xnb) << "\n";
  std::cout << "vcov=" << nbfit.vcov(xnb) << "\n";
  std::cout << "stderr=" << nbfit.stderr(xnb) << "\n";
  std::cout << "confint=" << nbfit.confint(xnb) << "\n";

  const auto nigfit = Nig{1.5, 0.1, 0.0, 1.0}.MLE(xheavy);
  std::cout << "\nNIG\n";
  std::cout << "alpha=" << nigfit.alpha << " beta=" << nigfit.beta
            << " location=" << nigfit.location << " scale=" << nigfit.scale << "\n";
  std::cout << "loglik=" << nigfit.loglik(xheavy) << "\n";
  std::cout << "loglik_per_obs=" << nigfit.loglik_per_obs(xheavy) << "\n";
  std::cout << "nll=" << nigfit.nll(xheavy) << "\n";
  std::cout << "score=" << nigfit.score(xheavy) << "\n";
  std::cout << "fisher_inf=" << nigfit.fisher_inf(xheavy) << "\n";
  std::cout << "observed_inf=" << nigfit.observed_inf(xheavy) << "\n";
  std::cout << "vcov=" << nigfit.vcov(xheavy) << "\n";
  std::cout << "stderr=" << nigfit.stderr(xheavy) << "\n";
  std::cout << "confint=" << nigfit.confint(xheavy) << "\n";

  const auto ghfit = Gh{1.0, 2.0, 0.1, 0.0, 1.0}.MLE(xheavy);
  std::cout << "\nGH\n";
  std::cout << "lambda=" << ghfit.lambda << " alpha=" << ghfit.alpha << " beta=" << ghfit.beta
            << " location=" << ghfit.location << " scale=" << ghfit.scale << "\n";
  std::cout << "loglik=" << ghfit.loglik(xheavy) << "\n";
  std::cout << "loglik_per_obs=" << ghfit.loglik_per_obs(xheavy) << "\n";
  std::cout << "nll=" << ghfit.nll(xheavy) << "\n";
  std::cout << "score=" << ghfit.score(xheavy) << "\n";
  std::cout << "fisher_inf=" << ghfit.fisher_inf(xheavy) << "\n";
  std::cout << "observed_inf=" << ghfit.observed_inf(xheavy) << "\n";
  std::cout << "vcov=" << ghfit.vcov(xheavy) << "\n";
  std::cout << "stderr=" << ghfit.stderr(xheavy) << "\n";
  std::cout << "confint=" << ghfit.confint(xheavy) << "\n";

  const Series w0 = pluss::make_series({0.5, 0.5}, f64);
  const Series m0 = pluss::make_series({-1.5, 1.5}, f64);
  const Series s0 = pluss::make_series({1.0, 1.0}, f64);
  const auto gmfit = GaussianMixture{w0, m0, s0}.MLE(xmix);
  std::cout << "\nGaussianMixture\n";
  std::cout << "weights=" << gmfit.weights << "\n";
  std::cout << "means=" << gmfit.means << "\n";
  std::cout << "stddevs=" << gmfit.stddevs << "\n";
  std::cout << "loglik=" << gmfit.loglik(xmix) << "\n";
  std::cout << "loglik_per_obs=" << gmfit.loglik_per_obs(xmix) << "\n";
  std::cout << "nll=" << gmfit.nll(xmix) << "\n";
  std::cout << "score=" << gmfit.score(xmix) << "\n";
  std::cout << "fisher_inf=" << gmfit.fisher_inf(xmix) << "\n";
  std::cout << "observed_inf=" << gmfit.observed_inf(xmix) << "\n";
  std::cout << "vcov=" << gmfit.vcov(xmix) << "\n";
  std::cout << "stderr=" << gmfit.stderr(xmix) << "\n";
  std::cout << "confint=" << gmfit.confint(xmix) << "\n";

  return 0;
}
