#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
namespace actuarial = pluss::ma::actuarial;
using namespace pluss::subsp::actuarial::loss_models;

double as_f64_scalar(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> as_f64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

bool approx(double a, double b, double tol = 1e-10) {
  return std::abs(a - b) <= tol;
}

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

} // namespace

int main() {
  int failures = 0;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  {
    const AggregateMoments pois = poisson_moments(2.5, pluss::pCPU);
    expect(approx(as_f64_scalar(pois.mean), 2.5, 1e-12), "poisson_moments mean", &failures);
    expect(approx(as_f64_scalar(pois.variance), 2.5, 1e-12), "poisson_moments variance", &failures);

    const AggregateMoments nb = negbin_moments(3.0, 0.4, pluss::pCPU);
    expect(approx(as_f64_scalar(nb.mean), 4.5, 1e-12), "negbin_moments mean", &failures);
    expect(approx(as_f64_scalar(nb.variance), 11.25, 1e-12), "negbin_moments variance", &failures);
  }

  {
    const AggregateMoments ln = lognormal_moments(0.0, 0.5, pluss::pCPU);
    const double expected_mean = std::exp(0.125);
    const double expected_var = (std::exp(0.25) - 1.0) * std::exp(0.25);
    expect(approx(as_f64_scalar(ln.mean), expected_mean, 1e-12), "lognormal_moments mean", &failures);
    expect(approx(as_f64_scalar(ln.variance), expected_var, 1e-12), "lognormal_moments variance", &failures);

    const AggregateMoments ga = gamma_moments(2.0, 3.0, 0.0, pluss::pCPU);
    expect(approx(as_f64_scalar(ga.mean), 6.0, 1e-12), "gamma_moments mean", &failures);
    expect(approx(as_f64_scalar(ga.variance), 18.0, 1e-12), "gamma_moments variance", &failures);
  }

  {
    const Series mN = pluss::make_series(2.0, cfg);
    const Series vN = pluss::make_series(2.0, cfg);
    const Series mX = pluss::make_series(10.0, cfg);
    const Series vX = pluss::make_series(25.0, cfg);

    const AggregateMoments agg = aggregate_moments(mN, vN, mX, vX);
    expect(approx(as_f64_scalar(agg.mean), 20.0, 1e-12), "aggregate mean", &failures);
    expect(approx(as_f64_scalar(agg.variance), 250.0, 1e-12), "aggregate variance", &failures);
  }

  {
    const AggregateMoments cp = compound_poisson(2.0, 10.0, 25.0, pluss::pCPU);
    expect(approx(as_f64_scalar(cp.mean), 20.0, 1e-12), "compound_poisson mean", &failures);
    expect(approx(as_f64_scalar(cp.variance), 250.0, 1e-12), "compound_poisson variance", &failures);

    const AggregateMoments cnb = compound_negbin(3.0, 0.5, 10.0, 25.0, pluss::pCPU);
    expect(approx(as_f64_scalar(cnb.mean), 30.0, 1e-12), "compound_negbin mean", &failures);
    expect(approx(as_f64_scalar(cnb.variance), 675.0, 1e-12), "compound_negbin variance", &failures);
  }

  {
    const auto pmf = as_f64_vec(poisson_pmf(2.0, 3, pluss::pCPU));
    expect(pmf.size() == 4, "poisson pmf size", &failures);
    expect(approx(pmf[0], std::exp(-2.0), 1e-12), "pmf[0]", &failures);
    expect(approx(pmf[1], 2.0 * std::exp(-2.0), 1e-12), "pmf[1]", &failures);
    expect(approx(pmf[2], 2.0 * std::exp(-2.0), 1e-12), "pmf[2]", &failures);
    expect(approx(pmf[3], (4.0 / 3.0) * std::exp(-2.0), 1e-12), "pmf[3]", &failures);

    const auto nb_pmf = as_f64_vec(negbin_pmf(2.0, 0.5, 2, pluss::pCPU));
    expect(nb_pmf.size() == 3, "negative binomial pmf size", &failures);
    expect(approx(nb_pmf[0], 0.25, 1e-12), "negative binomial pmf[0]", &failures);
    expect(approx(nb_pmf[1], 0.25, 1e-12), "negative binomial pmf[1]", &failures);
    expect(approx(nb_pmf[2], 0.1875, 1e-12), "negative binomial pmf[2]", &failures);
  }

  {
    const Series losses = pluss::make_series(std::vector<double>{50.0, 100.0, 200.0}, {3}, cfg);
    const auto ex = as_f64_vec(excess_loss(losses, 100.0));
    expect(ex.size() == 3, "excess_loss size", &failures);
    expect(approx(ex[0], 0.0, 1e-12) && approx(ex[1], 0.0, 1e-12) && approx(ex[2], 100.0, 1e-12),
           "excess_loss values",
           &failures);

    const auto layer = as_f64_vec(loss_layer(losses, 100.0, 40.0));
    expect(layer.size() == 3, "loss_layer size", &failures);
    expect(approx(layer[0], 0.0, 1e-12) && approx(layer[1], 0.0, 1e-12) && approx(layer[2], 40.0, 1e-12),
           "loss_layer values",
           &failures);

    const double layer_mean = as_f64_scalar(layer_premium(losses, 100.0, 40.0));
    expect(approx(layer_mean, 40.0 / 3.0, 1e-12), "layer_premium", &failures);

    const double stop = as_f64_scalar(stoploss(losses, 100.0));
    expect(approx(stop, 100.0 / 3.0, 1e-12), "stoploss", &failures);

    const actuarial::LossTransform contract =
        actuarial::operators::layer(100.0, 40.0, 0.5);
    const auto shared_layer = as_f64_vec(contract(losses));
    expect(contract.key() == "layer", "layer operator key", &failures);
    expect(approx(shared_layer[2], 20.0, 1e-12),
           "layer operator applies coinsurance", &failures);

    const actuarial::LossTransform capped = actuarial::operators::limit(75.0);
    const auto capped_losses = as_f64_vec(capped(losses));
    expect(approx(capped_losses[0], 50.0, 1e-12) &&
               approx(capped_losses[1], 75.0, 1e-12) &&
               approx(capped_losses[2], 75.0, 1e-12),
           "limit operator values", &failures);
  }

  {
    const double lev_exp = as_f64_scalar(
        gamma_lev(/*shape=*/1.0,
                                     /*scale=*/2.0,
                                     /*location=*/0.0,
                                     /*cap=*/3.0,
                                     /*quadrature_steps=*/4096,
                                     pluss::pCPU));
    const double expected_lev_exp = 2.0 * (1.0 - std::exp(-1.5));
    expect(approx(lev_exp, expected_lev_exp, 2e-4), "gamma_lev (exp closed form)", &failures);

    const double layer_exp = as_f64_scalar(
        gamma_layer(/*shape=*/1.0,
                            /*scale=*/2.0,
                            /*location=*/0.0,
                            /*attachment=*/1.0,
                            /*limit=*/2.0,
                            /*quadrature_steps=*/4096,
                            pluss::pCPU));
    const double expected_layer_exp = 2.0 * std::exp(-0.5) * (1.0 - std::exp(-1.0));
    expect(approx(layer_exp, expected_layer_exp, 2e-4), "gamma_layer (exp closed form)", &failures);
  }

  {
    const double lev_small = as_f64_scalar(
        lognormal_lev(0.1, 0.6, 1.0, 4096, pluss::pCPU));
    const double lev_big = as_f64_scalar(
        lognormal_lev(0.1, 0.6, 5.0, 4096, pluss::pCPU));
    expect(lev_small >= 0.0, "lognormal_lev nonnegative", &failures);
    expect(lev_big >= lev_small, "lognormal_lev monotone in cap", &failures);

    // A degenerate lognormal is the constant exp(mean_log).
    const double constant = std::exp(0.1);
    expect(approx(as_f64_scalar(lognormal_lev(0.1, 0.0, 5.0, 64, pluss::pCPU)), constant, 1e-12),
           "lognormal_lev with zero spread below the cap", &failures);
    expect(approx(as_f64_scalar(lognormal_lev(0.1, 0.0, 1.0, 64, pluss::pCPU)), 1.0, 1e-12),
           "lognormal_lev with zero spread above the cap", &failures);
    expect(approx(as_f64_scalar(lognormal_layer(0.1, 0.0, 0.5, 2.0, 64, pluss::pCPU)), constant - 0.5, 1e-12),
           "lognormal_layer with zero spread", &failures);

    const double layer = as_f64_scalar(
        lognormal_layer(0.1, 0.6, 1.0, 2.0, 4096, pluss::pCPU));
    expect(layer >= 0.0, "lognormal_layer nonnegative", &failures);
    expect(layer <= 2.0 + 1e-6, "lognormal_layer bounded by limit", &failures);
  }

  {
    bool threw = false;
    try {
      const Series losses = pluss::make_series(std::vector<double>{10.0, 20.0}, {2}, cfg);
      (void)stoploss(losses, -1.0);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "negative deductible should throw", &failures);
  }

  {
    bool threw = false;
    try {
      (void)negbin_moments(2.0, 1.2, pluss::pCPU);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "negbin_moments should reject p > 1", &failures);
  }

  return failures == 0 ? 0 : 1;
}
